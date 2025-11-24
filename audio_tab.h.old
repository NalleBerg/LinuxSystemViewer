#ifndef AUDIO_TAB_H
#define AUDIO_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class AudioTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit AudioTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_audioDevicesSection;
    QLabel* m_audioDevicesContent;
    
    QGroupBox* m_soundCardSection;
    QLabel* m_soundCardContent;
    
    QGroupBox* m_audioServerSection;
    QLabel* m_audioServerContent;
    
    QGroupBox* m_playbackSection;
    QLabel* m_playbackContent;
};

#endif // AUDIO_TAB_H
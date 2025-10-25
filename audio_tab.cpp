#include "audio_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

AudioTab::AudioTab(QWidget* parent)
    : TabWidgetBase("Audio", "lshw -C multimedia -short", true, 
                    "lshw -C multimedia && aplay -l 2>/dev/null && pactl info 2>/dev/null", parent)
{
    qDebug() << "AudioTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "AudioTab: Constructor finished";
}

QWidget* AudioTab::createUserFriendlyView()
{
    qDebug() << "AudioTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("Audio System Information");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("Audio Devices", &m_audioDevicesSection, &m_audioDevicesContent, mainLayout);
    createInfoSection("Sound Cards", &m_soundCardSection, &m_soundCardContent, mainLayout);
    createInfoSection("Audio Server", &m_audioServerSection, &m_audioServerContent, mainLayout);
    createInfoSection("Playback Devices", &m_playbackSection, &m_playbackContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "AudioTab: createUserFriendlyView completed";
    return scrollArea;
}

void AudioTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
{
    *groupBox = new QGroupBox(title);
    (*groupBox)->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 2px solid #bdc3c7;"
        "  border-radius: 8px;"
        "  margin-top: 10px;"
        "  padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 10px 0 10px;"
        "}"
    );
    
    QVBoxLayout* sectionLayout = new QVBoxLayout(*groupBox);
    *contentLabel = new QLabel("Loading " + title.toLower() + " information...");
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void AudioTab::parseOutput(const QString& output)
{
    qDebug() << "AudioTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString audioDevicesInfo = "Audio Devices: Not detected";
    QString soundCardInfo = "Sound Cards: Not detected";
    QString audioServerInfo = "Audio Server: Not detected";
    QString playbackInfo = "Playback Devices: Not detected";
    
    QStringList audioDevices;
    QStringList soundCards;
    QStringList playbackDevices;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lshw multimedia output
        if (trimmed.contains("multimedia") || trimmed.contains("audio")) {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString description = parts.mid(2).join(" ");
                if (!description.isEmpty() && !description.startsWith("H/W path")) {
                    audioDevices.append(description);
                }
            }
        }
        
        // Parse aplay -l output for sound cards
        if (trimmed.startsWith("card ")) {
            QString cardInfo = trimmed;
            cardInfo.replace(QRegularExpression("card \\d+:"), "Card:");
            soundCards.append(cardInfo);
        }
        
        // Parse PulseAudio info
        if (trimmed.startsWith("Server String:")) {
            audioServerInfo = "Audio Server: PulseAudio (" + trimmed.split(":")[1].trimmed() + ")";
        } else if (trimmed.startsWith("Server Version:")) {
            QString version = trimmed.split(":")[1].trimmed();
            if (audioServerInfo.contains("PulseAudio")) {
                audioServerInfo = "Audio Server: PulseAudio " + version;
            }
        } else if (trimmed.startsWith("Default Sink:")) {
            playbackDevices.append("Default: " + trimmed.split(":")[1].trimmed());
        }
        
        // Detect other audio servers
        if (trimmed.contains("pipewire") || trimmed.contains("PipeWire")) {
            audioServerInfo = "Audio Server: PipeWire";
        } else if (trimmed.contains("jack") || trimmed.contains("JACK")) {
            audioServerInfo = "Audio Server: JACK";
        }
    }
    
    // Format the information
    if (!audioDevices.isEmpty()) {
        audioDevicesInfo = "Audio Devices:\n" + audioDevices.join("\n");
    }
    
    if (!soundCards.isEmpty()) {
        soundCardInfo = "Sound Cards:\n" + soundCards.join("\n");
    }
    
    if (!playbackDevices.isEmpty()) {
        playbackInfo = "Playback Devices:\n" + playbackDevices.join("\n");
    }
    
    // Update the UI with parsed information
    if (m_audioDevicesContent) {
        m_audioDevicesContent->setText(audioDevicesInfo);
    }
    if (m_soundCardContent) {
        m_soundCardContent->setText(soundCardInfo);
    }
    if (m_audioServerContent) {
        m_audioServerContent->setText(audioServerInfo);
    }
    if (m_playbackContent) {
        m_playbackContent->setText(playbackInfo);
    }
    
    qDebug() << "AudioTab: parseOutput completed";
}
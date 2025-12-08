#ifndef AUDIO_H
#define AUDIO_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include "gui_helpers.h"

// Audio information functions
void loadAudioInformation(QTableWidget* table, const QJsonObject& data);
QStringList getAudioHeaders();
void styleAudioTable(QTableWidget* table);
QString getAudioInfo();

// Audio Headers
QStringList getAudioHeaders()
{
    return QStringList() << "Property" << "Value" << "Unit" << "Type";
}

// Audio Table Styling
void styleAudioTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 300);  // Value
    table->setColumnWidth(2, 80);   // Unit
    table->setColumnWidth(3, 120);  // Type
    
    // Style headers
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
}

// Load Audio Information
void loadAudioInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data); // Use direct system calls instead of JSON data
    
    table->setRowCount(0);
    
    // Read audio devices from /proc/asound/cards
    QFile cardsFile("/proc/asound/cards");
    if (cardsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&cardsFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.contains(QRegularExpression("^\\s*\\d+"))) {
                // Parse card line: " 0 [PCH            ]: HDA-Intel - HDA Intel PCH"
                QRegularExpression cardRegex("^\\s*(\\d+)\\s*\\[([^\\]]+)\\]\\s*:\\s*(.+)\\s*-\\s*(.+)");
                QRegularExpressionMatch match = cardRegex.match(line);
                if (match.hasMatch()) {
                    QString cardId = match.captured(1);
                    QString cardName = match.captured(2).trimmed();
                    QString driver = match.captured(3).trimmed();
                    QString description = match.captured(4).trimmed();
                    
                    addRowToTable(table, QStringList() << QString("Audio Card %1").arg(cardId) << description << "" << "Audio");
                    addRowToTable(table, QStringList() << QString("Card %1 Name").arg(cardId) << cardName << "" << "Audio");
                    addRowToTable(table, QStringList() << QString("Card %1 Driver").arg(cardId) << driver << "" << "Audio");
                }
            }
        }
        cardsFile.close();
    } else {
        addRowToTable(table, QStringList() << "Error" << "Could not read /proc/asound/cards" << "" << "Audio");
    }
    
    // Read audio devices from /proc/asound/devices
    QFile devicesFile("/proc/asound/devices");
    if (devicesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&devicesFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.contains("digital audio")) {
                // Parse device line
                QStringList parts = line.split(':');
                if (parts.size() >= 2) {
                    QString deviceInfo = parts[0].trimmed();
                    QString deviceType = parts[1].trimmed();
                    addRowToTable(table, QStringList() << "Audio Device" << QString("%1: %2").arg(deviceInfo, deviceType) << "" << "Audio");
                }
            }
        }
        devicesFile.close();
    }
    
    // Get PulseAudio information if available
    QProcess pulseProcess;
    pulseProcess.start("pactl", QStringList() << "info");
    pulseProcess.waitForFinished(3000);
    
    if (pulseProcess.exitCode() == 0) {
        QString pulseOutput = pulseProcess.readAllStandardOutput();
        QStringList pulseLines = pulseOutput.split('\n');
        
        for (const QString& line : pulseLines) {
            if (line.startsWith("Server String:")) {
                QString serverString = line.mid(14).trimmed();
                addRowToTable(table, QStringList() << "PulseAudio Server" << serverString << "" << "Audio");
            } else if (line.startsWith("Library Protocol Version:")) {
                QString version = line.mid(25).trimmed();
                addRowToTable(table, QStringList() << "PulseAudio Protocol" << version << "" << "Audio");
            } else if (line.startsWith("Server Protocol Version:")) {
                QString version = line.mid(24).trimmed();
                addRowToTable(table, QStringList() << "PulseAudio Server Protocol" << version << "" << "Audio");
            } else if (line.startsWith("Default Sink:")) {
                QString sink = line.mid(13).trimmed();
                addRowToTable(table, QStringList() << "Default Output Device" << sink << "" << "Audio");
            } else if (line.startsWith("Default Source:")) {
                QString source = line.mid(15).trimmed();
                addRowToTable(table, QStringList() << "Default Input Device" << source << "" << "Audio");
            }
        }
    } else {
        addRowToTable(table, QStringList() << "PulseAudio" << "Not available or not running" << "" << "Audio");
    }
    
    // Get ALSA version
    QProcess alsaProcess;
    alsaProcess.start("cat", QStringList() << "/proc/asound/version");
    alsaProcess.waitForFinished(3000);
    
    if (alsaProcess.exitCode() == 0) {
        QString alsaVersion = alsaProcess.readAllStandardOutput().trimmed();
        if (!alsaVersion.isEmpty()) {
            addRowToTable(table, QStringList() << "ALSA Version" << alsaVersion << "" << "Audio");
        }
    }
    
    // List PulseAudio sinks (output devices)
    QProcess sinkProcess;
    sinkProcess.start("pactl", QStringList() << "list" << "short" << "sinks");
    sinkProcess.waitForFinished(3000);
    
    if (sinkProcess.exitCode() == 0) {
        QString sinkOutput = sinkProcess.readAllStandardOutput();
        QStringList sinkLines = sinkOutput.split('\n');
        
        int sinkCount = 0;
        for (const QString& line : sinkLines) {
            if (!line.trimmed().isEmpty()) {
                QStringList parts = line.split('\t');
                if (parts.size() >= 2) {
                    QString sinkName = parts[1];
                    addRowToTable(table, QStringList() << QString("Output Device %1").arg(++sinkCount) << sinkName << "" << "Audio");
                }
            }
        }
    }
    
    // List PulseAudio sources (input devices)
    QProcess sourceProcess;
    sourceProcess.start("pactl", QStringList() << "list" << "short" << "sources");
    sourceProcess.waitForFinished(3000);
    
    if (sourceProcess.exitCode() == 0) {
        QString sourceOutput = sourceProcess.readAllStandardOutput();
        QStringList sourceLines = sourceOutput.split('\n');
        
        int sourceCount = 0;
        for (const QString& line : sourceLines) {
            if (!line.trimmed().isEmpty() && !line.contains(".monitor")) {
                QStringList parts = line.split('\t');
                if (parts.size() >= 2) {
                    QString sourceName = parts[1];
                    addRowToTable(table, QStringList() << QString("Input Device %1").arg(++sourceCount) << sourceName << "" << "Audio");
                }
            }
        }
    }
    
    // Check for JACK audio system
    QProcess jackProcess;
    jackProcess.start("jack_control", QStringList() << "status");
    jackProcess.waitForFinished(3000);
    
    if (jackProcess.exitCode() == 0) {
        QString jackOutput = jackProcess.readAllStandardOutput().trimmed();
        addRowToTable(table, QStringList() << "JACK Audio" << jackOutput << "" << "Audio");
    } else {
        addRowToTable(table, QStringList() << "JACK Audio" << "Not running" << "" << "Audio");
    }
    
    // Get audio mixer information
    QDir mixerDir("/proc/asound");
    QStringList mixerEntries = mixerDir.entryList(QStringList() << "card*", QDir::Dirs);
    
    for (const QString& cardDir : mixerEntries) {
        QString mixerPath = QString("/proc/asound/%1/codec#0").arg(cardDir);
        QFile mixerFile(mixerPath);
        if (mixerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&mixerFile);
            QString firstLine = stream.readLine();
            mixerFile.close();
            
            if (!firstLine.isEmpty()) {
                QString cardNum = cardDir.mid(4); // Remove "card" prefix
                addRowToTable(table, QStringList() << QString("Codec Info (Card %1)").arg(cardNum) << firstLine.trimmed() << "" << "Audio");
            }
        }
    }
}

// Get basic audio info string
QString getAudioInfo()
{
    QFile file("/proc/asound/cards");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "No audio information available";
    }
    
    QTextStream in(&file);
    QString firstLine = in.readLine();
    file.close();
    
    if (!firstLine.isEmpty() && firstLine.contains(QRegularExpression("^\\s*\\d+"))) {
        QRegularExpression cardRegex("^\\s*\\d+\\s*\\[([^\\]]+)\\]\\s*:\\s*(.+)\\s*-\\s*(.+)");
        QRegularExpressionMatch match = cardRegex.match(firstLine);
        if (match.hasMatch()) {
            return match.captured(3).trimmed(); // Description
        }
    }
    
    return "Audio hardware detected";
}

#endif // AUDIO_H
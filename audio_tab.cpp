#include "audio_tab.h"
#include "gui_helpers.h"
#include "alsa_player.h"
#include "geek_search_integration.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollArea>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QPainter>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QProcessEnvironment>
#include <thread>
#include <atomic>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTableWidgetItem>
#include <QFont>
#include <QMessageBox>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QThread>
#include <QCoreApplication>
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>
#include <QIODevice>
#include <QByteArray>
#include <QBuffer>
#include <QtMath>
#include <QEventLoop>
#include <QTimer>
#include <QListWidget>
#include <QDialog>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioTab::AudioTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Audio"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &AudioTab::showGeekMode);
    
    // Test Sound button - insert before Geek button
    testSoundButton = new QPushButton(tr("Sound-test"));
    testSoundButton->setStyleSheet(geekButton->styleSheet()); // Same style as Geek button
    connect(testSoundButton, &QPushButton::clicked, this, &AudioTab::testSound);
    headlineLayout->insertWidget(headlineLayout->count() - 1, testSoundButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setColumnWidth(0, 220);  // Match CPU tab width

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Populate table
    loadAudioInfo();

    // Auto-refresh every 3 seconds (for USB audio device changes)
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(3000);
    connect(refreshTimer, &QTimer::timeout, this, &AudioTab::refreshValues);
    
    enableTableCopy(tableWidget, refreshTimer);
}

void AudioTab::showEvent(QShowEvent* ev)
{
    QWidget::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void AudioTab::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QWidget::hideEvent(ev);
}

void AudioTab::showGeekMode()
{
    // Working inline dialog approach
    QDialog* loading = new QDialog(this);
    loading->setWindowTitle(tr("Loading"));
    loading->setModal(true);
    loading->setFixedSize(300, 100);
    
    QVBoxLayout* layout = new QVBoxLayout(loading);
    QLabel* label = new QLabel(tr("Scanning audio devices, please wait..."), loading);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    loading->show();
    QApplication::processEvents();
    
    // Create and populate dialog while progress is showing
    AudioGeekDialog dlg(this);
    
    // Close loading dialog
    loading->close();
    delete loading;
    
    // Show the actual geek mode dialog
    dlg.exec();
}

void AudioTab::testSound()
{
    qDebug() << "AudioTab::testSound() called!";
    fprintf(stderr, "=== AUDIO TEST STARTING ===\n");
    fflush(stderr);
    
    // Create audio test dialog with checklist
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Audio Test"));
    dialog->setModal(true);
    dialog->resize(400, 220);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QLabel* infoLabel = new QLabel(tr("Testing audio output with generated tones:"));
    infoLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin-bottom: 10px;");
    layout->addWidget(infoLabel);
    
    QListWidget* checkList = new QListWidget();
    checkList->addItem("⏳ Testing A440 Hz (musical A)...");
    checkList->addItem("⏳ Testing Low C (261.63 Hz)...");
    checkList->addItem("⏳ Testing Left Speaker...");
    checkList->addItem("⏳ Testing Right Speaker...");
    layout->addWidget(checkList);
    
    bool cancelled = false;
    QPushButton* stopBtn = new QPushButton(tr("Stop"));
    connect(stopBtn, &QPushButton::clicked, [&cancelled, dialog]() {
        cancelled = true;
        dialog->reject();  // Close dialog immediately
    });
    layout->addWidget(stopBtn);
    
    dialog->show();
    QCoreApplication::processEvents();
    
    // Run tests
    playTestSound(checkList, stopBtn, &cancelled);
    
    // If not cancelled, change button to Close and wait for user
    if (!cancelled) {
        stopBtn->setText(tr("Close"));
        connect(stopBtn, &QPushButton::clicked, dialog, &QDialog::accept);
        dialog->exec();
    }
    
    delete dialog;
}

void AudioTab::playTestSound(QListWidget* checkList, QPushButton* stopBtn, bool* cancelFlag)
{
    // Sound file paths - try install locations first, fall back to source
    QString soundPath = "/usr/share/lsv/sounds/";
    fprintf(stderr, "\n=== AUDIO TEST DEBUG START ===\n");
    fprintf(stderr, "Checking path 1: %s\n", soundPath.toStdString().c_str());
    fflush(stderr);
    
    if (!QFile::exists(soundPath + "a440.wav")) {
        fprintf(stderr, "  NOT FOUND, trying path 2...\n");
        fflush(stderr);
        soundPath = "/usr/local/share/lsv/sounds/";
        fprintf(stderr, "Checking path 2: %s\n", soundPath.toStdString().c_str());
        fflush(stderr);
    } else {
        fprintf(stderr, "  FOUND!\n");
        fflush(stderr);
    }
    
    if (!QFile::exists(soundPath + "a440.wav")) {
        fprintf(stderr, "  NOT FOUND, trying path 3...\n");
        fflush(stderr);
        soundPath = QCoreApplication::applicationDirPath() + "/../sounds/";
        fprintf(stderr, "Checking path 3: %s\n", soundPath.toStdString().c_str());
        fflush(stderr);
    } else {
        fprintf(stderr, "  FOUND!\n");
        fflush(stderr);
    }
    
    fprintf(stderr, "Final sound path: %s\n", soundPath.toStdString().c_str());
    fflush(stderr);
    
    int currentTest = 0;
    
    // Test A440 Hz tone
    if (*cancelFlag) goto cleanup;
    checkList->item(0)->setText("▶ Testing A440 Hz (musical A)...");
    QCoreApplication::processEvents();
    currentTest = 0;
    
    playSoundFile(soundPath + "a440.wav", cancelFlag);
    if (*cancelFlag) goto cleanup;
    checkList->item(0)->setText("✓ A440 Hz test complete");
    QCoreApplication::processEvents();
    
    for (int i = 0; i < 10 && !*cancelFlag; ++i) {
        QThread::msleep(50);
        QCoreApplication::processEvents();
    }
    
    // Test Low C tone
    if (*cancelFlag) goto cleanup;
    checkList->item(1)->setText("▶ Testing Low C (261.63 Hz)...");
    QCoreApplication::processEvents();
    currentTest = 1;
    
    playSoundFile(soundPath + "lowc.wav", cancelFlag);
    if (*cancelFlag) goto cleanup;
    checkList->item(1)->setText("✓ Low C test complete");
    QCoreApplication::processEvents();
    
    for (int i = 0; i < 10 && !*cancelFlag; ++i) {
        QThread::msleep(50);
        QCoreApplication::processEvents();
    }
    
    // Test left speaker
    if (*cancelFlag) goto cleanup;
    checkList->item(2)->setText("▶ Testing Left Speaker...");
    QCoreApplication::processEvents();
    currentTest = 2;
    
    playSoundFile(soundPath + "left.wav", cancelFlag);
    if (*cancelFlag) goto cleanup;
    checkList->item(2)->setText("✓ Left speaker test complete");
    QCoreApplication::processEvents();
    
    for (int i = 0; i < 10 && !*cancelFlag; ++i) {
        QThread::msleep(50);
        QCoreApplication::processEvents();
    }
    
    // Test right speaker
    if (*cancelFlag) goto cleanup;
    checkList->item(3)->setText("▶ Testing Right Speaker...");
    QCoreApplication::processEvents();
    currentTest = 3;
    
    playSoundFile(soundPath + "right.wav", cancelFlag);
    if (*cancelFlag) goto cleanup;
    checkList->item(3)->setText("✓ Right speaker test complete");
    QCoreApplication::processEvents();
    
cleanup:
    // Mark all remaining tests as cancelled
    if (*cancelFlag) {
        for (int i = currentTest; i < checkList->count(); ++i) {
            QString text = checkList->item(i)->text();
            if (!text.startsWith("✓")) {
                checkList->item(i)->setText("✗ Cancelled");
            }
        }
    }
}

void AudioTab::playSoundFile(const QString& filename, bool* cancelFlag)
{
    fprintf(stderr, "playSoundFile called with: %s\n", filename.toStdString().c_str());
    fflush(stderr);
    
    if (!QFile::exists(filename)) {
        fprintf(stderr, "FILE NOT FOUND: %s\n", filename.toStdString().c_str());
        fflush(stderr);
        QMessageBox::warning(this, "Sound Error", "Sound file not found:\n" + filename);
        return;
    }
    
    fprintf(stderr, "File exists, using ALSA with cancellation support...\n");
    fflush(stderr);
    
    // Run ALSA player in a separate thread, passing the cancel flag
    std::atomic<bool> alsaFinished(false);
    std::atomic<bool> alsaSuccess(false);
    
    std::thread alsaThread([&filename, &alsaFinished, &alsaSuccess, cancelFlag]() {
        bool result = AlsaPlayer::playWavFile(filename.toStdString(), cancelFlag);
        alsaSuccess.store(result);
        alsaFinished.store(true);
    });
    
    // Wait for ALSA to finish
    alsaThread.join();
    
    fprintf(stderr, "ALSA playback completed: %d (cancelled: %d)\n", alsaSuccess.load(), *cancelFlag);
    fflush(stderr);
    
    fprintf(stderr, "Sound playback finished or cancelled\n");
    fflush(stderr);
}

void AudioTab::generateAndPlayTone(double frequency, double duration, double leftVolume, double rightVolume)
{
    // Try Qt Multimedia first (native, no external dependencies)
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    
    if (!device.isNull() && device.isFormatSupported(format)) {
        // Generate audio data
        int sampleCount = static_cast<int>(format.sampleRate() * duration);
        QByteArray audioData;
        audioData.resize(sampleCount * format.channelCount() * sizeof(qint16));
        qint16* data = reinterpret_cast<qint16*>(audioData.data());
        
        double amplitude = 32767.0 * 0.3;
        
        for (int i = 0; i < sampleCount; ++i) {
            double t = static_cast<double>(i) / format.sampleRate();
            double sample = amplitude * std::sin(2.0 * M_PI * frequency * t);
            data[i * 2] = static_cast<qint16>(sample * leftVolume);
            data[i * 2 + 1] = static_cast<qint16>(sample * rightVolume);
        }
        
        // Play using Qt Multimedia
        QAudioSink* audioSink = new QAudioSink(device, format);
        QBuffer* buffer = new QBuffer();
        buffer->setData(audioData);
        buffer->open(QIODevice::ReadOnly);
        
        audioSink->start(buffer);
        
        while (audioSink->state() == QAudio::ActiveState) {
            QCoreApplication::processEvents();
            QThread::msleep(50);
        }
        
        audioSink->stop();
        delete buffer;
        delete audioSink;
        return;
    }
    
    // Fallback: use system audio players
    // Generate WAV data
    const int sampleRate = 44100;
    const int channels = 2;
    const int bitsPerSample = 16;
    int sampleCount = static_cast<int>(sampleRate * duration);
    
    QByteArray audioData;
    audioData.resize(sampleCount * channels * sizeof(qint16));
    qint16* data = reinterpret_cast<qint16*>(audioData.data());
    
    double amplitude = 32767.0 * 0.5;
    
    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = amplitude * std::sin(2.0 * M_PI * frequency * t);
        data[i * 2] = static_cast<qint16>(sample * leftVolume);
        data[i * 2 + 1] = static_cast<qint16>(sample * rightVolume);
    }
    
    // Build WAV file
    QByteArray wavData;
    int dataSize = audioData.size();
    int fileSize = dataSize + 36;
    
    wavData.append("RIFF", 4);
    wavData.append((char*)&fileSize, 4);
    wavData.append("WAVE", 4);
    wavData.append("fmt ", 4);
    int fmtSize = 16;
    wavData.append((char*)&fmtSize, 4);
    short format2 = 1;
    wavData.append((char*)&format2, 2);
    wavData.append((char*)&channels, 2);
    wavData.append((char*)&sampleRate, 4);
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    wavData.append((char*)&byteRate, 4);
    short blockAlign = channels * bitsPerSample / 8;
    wavData.append((char*)&blockAlign, 2);
    wavData.append((char*)&bitsPerSample, 2);
    wavData.append("data", 4);
    wavData.append((char*)&dataSize, 4);
    wavData.append(audioData);
    
    // Write to temp file
    QString tempFile = QDir::temp().filePath("lsv_audio_test.wav");
    QFile file(tempFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(wavData);
        file.close();
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        
        // Try paplay first
        int result = QProcess::execute("paplay", QStringList() << tempFile);
        if (result != 0) {
            // Try aplay
            result = QProcess::execute("aplay", QStringList() << "-q" << tempFile);
            if (result != 0) {
                // Try play (sox)
                QProcess::execute("play", QStringList() << tempFile);
            }
        }
        
        // Keep file for debugging - will be overwritten next time
        // QFile::remove(tempFile);
    } else {
        QMessageBox::warning(this, "Debug", "Could not create temp file: " + tempFile);
    }
}

void AudioTab::refreshValues()
{
    loadAudioInfo();
}

void AudioTab::loadAudioInfo()
{
    tableWidget->setRowCount(0);
    
    auto addRow = [&](const QString& prop, const QString& val){
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        QTableWidgetItem* propItem = new QTableWidgetItem(prop);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        tableWidget->setItem(row, 0, propItem);
        tableWidget->setItem(row, 1, new QTableWidgetItem(val));
        tableWidget->resizeRowToContents(row);
    };
    
    // Detect audio server
    QString audioServer = "Unknown";
    QProcess serverProc;
    serverProc.start("sh", QStringList() << "-c" << "pactl info 2>/dev/null | grep 'Server Name' | cut -d: -f2");
    serverProc.waitForFinished(1000);
    QString serverOutput = QString::fromLocal8Bit(serverProc.readAllStandardOutput()).trimmed();
    if (!serverOutput.isEmpty()) {
        if (serverOutput.contains("PulseAudio")) {
            audioServer = "PulseAudio";
            // Get version
            QProcess verProc;
            verProc.start("sh", QStringList() << "-c" << "pactl info 2>/dev/null | grep 'Server Version' | cut -d: -f2");
            verProc.waitForFinished(500);
            QString version = QString::fromLocal8Bit(verProc.readAllStandardOutput()).trimmed();
            if (!version.isEmpty()) {
                audioServer += " " + version;
            }
        } else if (serverOutput.contains("PipeWire")) {
            audioServer = "PipeWire";
        }
    } else {
        // Check for PipeWire
        QProcess pwProc;
        pwProc.start("pgrep", QStringList() << "-x" << "pipewire");
        pwProc.waitForFinished(500);
        if (pwProc.exitCode() == 0) {
            audioServer = "PipeWire";
        }
    }
    addRow(tr("Audio Server"), audioServer);
    
    // Get default sink/output device
    QProcess sinkProc;
    sinkProc.start("sh", QStringList() << "-c" << "pactl info 2>/dev/null | grep 'Default Sink' | cut -d: -f2");
    sinkProc.waitForFinished(1000);
    QString defaultSink = QString::fromLocal8Bit(sinkProc.readAllStandardOutput()).trimmed();
    if (!defaultSink.isEmpty()) {
        addRow(tr("Default Output"), defaultSink);
    }
    
    // Get default source/input device
    QProcess sourceProc;
    sourceProc.start("sh", QStringList() << "-c" << "pactl info 2>/dev/null | grep 'Default Source' | cut -d: -f2");
    sourceProc.waitForFinished(1000);
    QString defaultSource = QString::fromLocal8Bit(sourceProc.readAllStandardOutput()).trimmed();
    if (!defaultSource.isEmpty()) {
        addRow(tr("Default Input"), defaultSource);
    }
    
    // List all audio cards from /proc/asound/cards
    QFile cardsFile("/proc/asound/cards");
    if (cardsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cardsFile);
        int cardNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("[") && line.contains("]")) {
                // Card definition line like "0 [PCH]: HDA-Intel - HDA Intel PCH"
                QRegularExpression re("\\[(\\w+)\\]:\\s*(.+)");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch()) {
                    QString cardId = match.captured(1);
                    QString cardInfo = match.captured(2);
                    addRow(tr("Sound Card %1").arg(cardNum++), cardInfo);
                }
            }
        }
        cardsFile.close();
    }
    
    // List PCM devices from aplay
    QProcess aplayProc;
    aplayProc.start("aplay", QStringList() << "-l");
    aplayProc.waitForFinished(2000);
    QString aplayOutput = QString::fromLocal8Bit(aplayProc.readAllStandardOutput());
    if (!aplayOutput.isEmpty()) {
        QStringList lines = aplayOutput.split('\n', Qt::SkipEmptyParts);
        int deviceNum = 0;
        for (const QString& line : lines) {
            if (line.startsWith("card ")) {
                // Parse line like "card 0: PCH [HDA Intel PCH], device 0: ALC3246 Analog [ALC3246 Analog]"
                QRegularExpression re("card\\s+(\\d+):\\s+([^,]+),\\s+device\\s+(\\d+):\\s+(.+)");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch()) {
                    QString cardNum = match.captured(1);
                    QString cardName = match.captured(2).trimmed();
                    QString devNum = match.captured(3);
                    QString devName = match.captured(4).trimmed();
                    addRow(tr("Playback Device %1").arg(deviceNum++), 
                           QString("hw:%1,%2 - %3").arg(cardNum).arg(devNum).arg(devName));
                }
            }
        }
    }
}

// --- AudioGeekDialog ---

AudioGeekDialog::AudioGeekDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Audio - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Audio System Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Button layout: Rescan on left, Copy/Save/Close on right
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* rescanBtn = new QPushButton(tr("Rescan"));
    buttonLayout->addWidget(rescanBtn);
    
    // Add search button using the integration helper
    GeekSearchIntegration::addSearchButtonToGeekDialog(buttonLayout, this, table);
    
    buttonLayout->addStretch();
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(buttonBox);
    
    // Fix Close button translation - apply after dialog is shown
    QTimer::singleShot(0, [buttonBox, this]() {
        if (auto closeBtn = buttonBox->button(QDialogButtonBox::Close)) {
            closeBtn->setText(tr("Close"));
        }
    });
    
    layout->addLayout(buttonLayout);

    enableTableCopy(table, nullptr);

    connect(rescanBtn, &QPushButton::clicked, this, &AudioGeekDialog::rescan);
    connect(copyBtn, &QPushButton::clicked, this, &AudioGeekDialog::copyToClipboard);
    connect(saveBtn, &QPushButton::clicked, this, &AudioGeekDialog::saveToFile);

    fillTable();
}

void AudioGeekDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
}

void AudioGeekDialog::hideEvent(QHideEvent* ev)
{
    QDialog::hideEvent(ev);
}

void AudioGeekDialog::rescan()
{
    // Working inline dialog approach
    QDialog* loading = new QDialog(this);
    loading->setWindowTitle(tr("Loading"));
    loading->setModal(true);
    loading->setFixedSize(300, 100);
    
    QVBoxLayout* layout = new QVBoxLayout(loading);
    QLabel* label = new QLabel(tr("Rescanning audio devices, please wait..."), loading);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    loading->show();
    QApplication::processEvents();
    
    // Refresh the data
    fillTable();
    
    // Close loading dialog
    loading->close();
    delete loading;
}

void AudioGeekDialog::copyToClipboard()
{
    QString all;
    for (int r = 0; r < table->rowCount(); ++r) {
        QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
        QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
        all += prop + ": " + val + "\n";
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(all, QClipboard::Clipboard);
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Copied"));
    msgBox.setText(tr("The information has been copied\nto the clipboard."));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void AudioGeekDialog::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Audio Info"), "audio-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty()) return;

    auto esc = [](const QString &s)->QString {
        QString out = s;
        out.replace('"', "\"\"");
        if (out.contains(',') || out.contains('\n') || out.contains('"')) {
            out = '"' + out + '"';
        }
        return out;
    };

    QFile out(fileName);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream ts(&out);
    ts << "Property,Value\n";
    for (int r = 0; r < table->rowCount(); ++r) {
        QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
        QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
        ts << esc(prop) << ',' << esc(val) << '\n';
    }
    out.close();
}

void AudioGeekDialog::fillTable()
{
    table->setRowCount(0);
    
    auto addRow = [&](const QString& prop, const QString& val){
        int row = table->rowCount();
        table->insertRow(row);
        QTableWidgetItem* propItem = new QTableWidgetItem(prop);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        table->setItem(row, 0, propItem);
        table->setItem(row, 1, new QTableWidgetItem(val));
    };
    
    auto addSection = [&](const QString& title) {
        int row = table->rowCount();
        table->insertRow(row);
        QTableWidgetItem* sectionItem = new QTableWidgetItem(title);
        sectionItem->setBackground(QBrush(QColor("#ecf0f1")));
        QFont boldFont = sectionItem->font();
        boldFont.setBold(true);
        sectionItem->setFont(boldFont);
        table->setItem(row, 0, sectionItem);
        table->setItem(row, 1, new QTableWidgetItem(""));
    };
    
    // === Audio Server ===
    addSection("=== Audio Server ===");
    
    QProcess pactlProc;
    pactlProc.start("pactl", QStringList() << "info");
    pactlProc.waitForFinished(2000);
    QString pactlOutput = QString::fromLocal8Bit(pactlProc.readAllStandardOutput());
    if (!pactlOutput.isEmpty()) {
        QStringList lines = pactlOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.contains(':')) {
                QStringList parts = line.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    addRow(parts[0].trimmed(), parts[1].trimmed());
                }
            }
        }
    }
    
    // === Sound Cards (/proc/asound/cards) ===
    addSection("=== Sound Cards (/proc/asound/cards) ===");
    
    QFile cardsFile("/proc/asound/cards");
    if (cardsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cardsFile);
        QString cardInfo;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.trimmed().isEmpty()) {
                if (line.startsWith(" ")) {
                    cardInfo += " " + line.trimmed();
                } else {
                    if (!cardInfo.isEmpty()) {
                        addRow("Card", cardInfo);
                    }
                    cardInfo = line.trimmed();
                }
            }
        }
        if (!cardInfo.isEmpty()) {
            addRow("Card", cardInfo);
        }
        cardsFile.close();
    }
    
    // === ALSA Playback Devices (aplay -l) ===
    addSection("=== ALSA Playback Devices (aplay -l) ===");
    
    QProcess aplayProc;
    aplayProc.start("aplay", QStringList() << "-l");
    aplayProc.waitForFinished(2000);
    QString aplayOutput = QString::fromLocal8Bit(aplayProc.readAllStandardOutput());
    if (!aplayOutput.isEmpty()) {
        QStringList lines = aplayOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.startsWith("card ")) {
                addRow("Device", line.trimmed());
            } else if (line.contains("Subdevices:")) {
                addRow("Subdevices", line.trimmed());
            }
        }
    }
    
    // === ALSA Recording Devices (arecord -l) ===
    addSection("=== ALSA Recording Devices (arecord -l) ===");
    
    QProcess arecordProc;
    arecordProc.start("arecord", QStringList() << "-l");
    arecordProc.waitForFinished(2000);
    QString arecordOutput = QString::fromLocal8Bit(arecordProc.readAllStandardOutput());
    if (!arecordOutput.isEmpty()) {
        QStringList lines = arecordOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.startsWith("card ")) {
                addRow("Device", line.trimmed());
            } else if (line.contains("Subdevices:")) {
                addRow("Subdevices", line.trimmed());
            }
        }
    }
    
    // === PulseAudio/PipeWire Sinks ===
    addSection("=== Output Devices (Sinks) ===");
    
    QProcess sinksProc;
    sinksProc.start("sh", QStringList() << "-c" << "pactl list sinks 2>/dev/null | grep -E '(Name:|Description:|State:|Volume:|Mute:)'");
    sinksProc.waitForFinished(3000);
    QString sinksOutput = QString::fromLocal8Bit(sinksProc.readAllStandardOutput());
    if (!sinksOutput.isEmpty()) {
        QStringList lines = sinksOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    addRow(parts[0].trimmed(), parts[1].trimmed());
                }
            }
        }
    }
    
    // === PulseAudio/PipeWire Sources ===
    addSection("=== Input Devices (Sources) ===");
    
    QProcess sourcesProc;
    sourcesProc.start("sh", QStringList() << "-c" << "pactl list sources 2>/dev/null | grep -E '(Name:|Description:|State:|Volume:|Mute:)'");
    sourcesProc.waitForFinished(3000);
    QString sourcesOutput = QString::fromLocal8Bit(sourcesProc.readAllStandardOutput());
    if (!sourcesOutput.isEmpty()) {
        QStringList lines = sourcesOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    addRow(parts[0].trimmed(), parts[1].trimmed());
                }
            }
        }
    }
    
    // === Hardware Info (lshw -C multimedia) ===
    addSection("=== Hardware Information (lshw) ===");
    
    QProcess lshwProc;
    lshwProc.start("sh", QStringList() << "-c" << "lshw -C multimedia 2>/dev/null | grep -E '(product:|vendor:|driver:|version:|bus info:|capabilities:)'");
    lshwProc.waitForFinished(3000);
    QString lshwOutput = QString::fromLocal8Bit(lshwProc.readAllStandardOutput());
    if (!lshwOutput.isEmpty()) {
        QStringList lines = lshwOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    addRow(parts[0].trimmed(), parts[1].trimmed());
                }
            }
        }
    }
    
    // === Module Information ===
    addSection("=== Kernel Modules ===");
    
    QProcess lsmodProc;
    lsmodProc.start("sh", QStringList() << "-c" << "lsmod | grep -E '(snd|audio)' | awk '{print $1}'");
    lsmodProc.waitForFinished(2000);
    QString modulesOutput = QString::fromLocal8Bit(lsmodProc.readAllStandardOutput());
    if (!modulesOutput.isEmpty()) {
        QStringList modules = modulesOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& module : modules) {
            addRow("Module", module.trimmed());
        }
    }
}

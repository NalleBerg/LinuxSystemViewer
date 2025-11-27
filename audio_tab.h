#ifndef AUDIO_TAB_H
#define AUDIO_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>
#include <QDialog>
#include <QListWidget>
#include <QProcess>

class AudioGeekDialog;

class AudioTab : public QWidget
{
    Q_OBJECT

public:
    explicit AudioTab(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private slots:
    void showGeekMode();
    void testSound();
    void refreshValues();

private:
    void loadAudioInfo();
    void playTestSound(class QListWidget* checkList, class QPushButton* closeBtn, bool* cancelFlag);
    void playSoundFile(const QString& filename, bool* cancelFlag);
    void generateAndPlayTone(double frequency, double duration, double leftVolume, double rightVolume);
    
    QTableWidget* tableWidget;
    QTimer* refreshTimer;
    QPushButton* geekButton;
    QPushButton* testSoundButton;
    QProcess* soundProcess;
};

// --- Geek Mode Dialog ---
class AudioGeekDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AudioGeekDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private slots:
    void copyToClipboard();
    void saveToFile();
    void rescan();

private:
    void fillTable();
    QTableWidget* table;
};

#endif // AUDIO_TAB_H

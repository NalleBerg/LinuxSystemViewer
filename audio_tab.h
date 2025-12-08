#ifndef AUDIO_TAB_H
#define AUDIO_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>
#include <QDialog>
#include <QListWidget>
#include <QProcess>
#include <QLabel>
#include <QStringList>

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
    
    // Loading spinner for main tab
    QLabel* loadingLabel;
    QLabel* spinnerLabel;
    QTimer* spinnerTimer;
    int spinnerIndex;
    const QStringList spinnerChars = {"\u280b", "\u2819", "\u2839", "\u2838", "\u283c", "\u2834", "\u2826", "\u2827", "\u2807", "\u280f"};
    
    void showMainTabSpinner();
    void hideMainTabSpinner();
    void updateMainTabSpinner();
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
    void showSpinner();
    void hideSpinner();
};

#endif // AUDIO_TAB_H

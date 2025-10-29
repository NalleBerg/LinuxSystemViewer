#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <sys/stat.h>
#include <pwd.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include <QTimer>
#include <QScreen>
#include <QIcon>
#include <QFile>
#include <QStyle>
#include <unistd.h> // For geteuid()
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QStringList>
#include <QFileInfo>
#include <QProgressBar>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

// Forward-declare appendLog from log_helper.h
#include "log_helper.h"
// Central version header (single source of truth for the version string)
#include "version.h"

static bool polkitAgentRunning()
{
    // Look for common polkit GUI auth agent process names
    QProcess p;
    p.start("ps", QStringList() << "-eo" << "cmd");
    if (!p.waitForFinished(1000)) return false;
    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    QStringList agents = {"polkit-gnome-authentication-agent-1", "polkit-mate-authentication-agent-1", "polkit-kde-authentication-agent-1", "polkit-gnome"};
    for (const QString &a : agents) {
        if (out.contains(a)) return true;
    }
    return false;
}

static QString detectDistroInstallCmds()
{
    QFile f("/etc/os-release");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QString s = QString::fromLocal8Bit(f.readAll());
    f.close();
    QString id;
    for (const QString &line : s.split('\n')) {
        if (line.startsWith("ID=", Qt::CaseInsensitive)) {
            id = line.mid(3).trimmed();
            if (id.startsWith('"') && id.endsWith('"') && id.length() >= 2) id = id.mid(1, id.length()-2);
            id = id.toLower();
            break;
        }
    }
    if (id.isEmpty()) return QString();
    if (id.contains("ubuntu") || id.contains("debian")) {
        return QString("sudo apt update && sudo apt install policykit-1-gnome\n# then log out and back in (or run: /usr/lib/policykit-1-gnome/polkit-gnome-authentication-agent-1 &)");
    } else if (id.contains("fedora") || id.contains("rhel") || id.contains("centos")) {
        return QString("sudo dnf install polkit-gnome -y\n# then log out and back in (or run: /usr/libexec/polkit-gnome-authentication-agent-1 &)");
    } else if (id.contains("arch")) {
        return QString("sudo pacman -S polkit-gnome\n# then log out and back in (or run: /usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1 &)");
    }
    return QString("Please install a polkit authentication agent for your desktop (policykit-1-gnome, mate-polkit, polkit-kde) and log out/in.");
}

static QPixmap makeBadgePixmap(const QColor &bgColor, int size = 20)
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(bgColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSizeF(size * 0.6);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(pix.rect(), Qt::AlignCenter, "i");
    p.end();
    return pix;
}

// Qt message handler: route Qt debug/info/warning messages into the
// appendLog file instead of printing to the console.
static void lsvQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QString prefix;
    switch (type) {
        case QtDebugMsg: prefix = "DEBUG: "; break;
        case QtInfoMsg: prefix = "INFO: "; break;
        case QtWarningMsg: prefix = "WARNING: "; break;
        case QtCriticalMsg: prefix = "CRITICAL: "; break;
        case QtFatalMsg: prefix = "FATAL: "; break;
        default: prefix = "LOG: "; break;
    }
    appendLog(prefix + msg);
    if (type == QtFatalMsg) {
        abort();
    }
}

#include "multitabs.h"
#include "ctrlw.h"
#include "tab_widget_base.h"
#include "summary_tab.h"
#include "generic_tab.h"
#include "os_tab.h"
#include "audio_tab.h"
#include "windowing_tab.h"
#include "graphics_tab.h"
#include "screen_tab.h"
#include "ports_tab.h"
#include "peripherals_tab.h"
#include "motherboard_tab.h"
#include "storage_tab.h"
#include "about_tab.h"
#include "cpu_tab.h"
#include "network_tab.h"
#include "tabs_config.h"
#include "pc_tab.h"
#include "memory_tab.h"
#include "log_helper.h"

// Perform cleanup of temporary files the application may have created.
static void performCleanup()
{
    appendLog("Cleaner: starting cleanup of temporary files");
    QDir tmpDir(QDir::tempPath());
    QDateTime now = QDateTime::currentDateTime();

    // Helper: check whether a file is owned by the current effective user
    auto ownedByCurrentUser = [](const QString &path) -> bool {
        struct stat st;
        if (stat(path.toUtf8().constData(), &st) != 0) return false;
        uid_t owner = st.st_uid;
        return owner == geteuid();
    };

    // Safety thresholds
    const qint64 MAX_REMOVE_SIZE = 5LL * 1024 * 1024; // 5 MB: do not remove big files automatically
    const int MIN_AGE_SECS = 5; // do not touch files younger than this to avoid races
    const int PRESERVE_IF_LARGER_THAN = 1024 * 1024; // 1 MB preserve threshold for certain logs

    qint64 removedCount = 0;
    qint64 freedBytes = 0;

    // Broadly target files and small directories that begin with "lsv-" in /tmp
    QFileInfoList candidates = tmpDir.entryInfoList(QStringList() << "lsv-*" << "lsv_*", QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : candidates) {
        const QString path = fi.absoluteFilePath();

        // Only operate inside the temp dir and on items owned by this user
        if (!ownedByCurrentUser(path)) {
            appendLog(QString("Cleaner: skipping (not owned by user) %1").arg(path));
            continue;
        }

        // Skip very recent files to avoid races with other running instances
        if (fi.lastModified().secsTo(now) <= MIN_AGE_SECS) {
            appendLog(QString("Cleaner: skipping recent file/dir %1").arg(path));
            continue;
        }

        if (fi.isDir()) {
            // Remove only empty lsv-* directories (safe) and report results
            QDir d(path);
            QStringList children = d.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
            if (children.isEmpty()) {
                appendLog(QString("Cleaner: removing empty temp dir %1").arg(path));
                if (d.rmdir(path)) {
                    removedCount++;
                } else {
                    appendLog(QString("Cleaner: failed to remove dir %1").arg(path));
                }
            } else {
                appendLog(QString("Cleaner: preserving non-empty dir %1 (entries=%2)").arg(path).arg(children.size()));
            }
            continue;
        }

        // It's a file. Make removal decisions based on size and name.
        qint64 sz = fi.size();

        // Preserve very large files for user inspection
        if (sz > MAX_REMOVE_SIZE) {
            appendLog(QString("Cleaner: preserving large file %1 (size=%2)").arg(path).arg(sz));
            continue;
        }

        // Preserve certain logs if they are larger than a threshold
        if (path.endsWith("lsv-about-links.log") && sz > PRESERVE_IF_LARGER_THAN) {
            appendLog(QString("Cleaner: preserving about-links log %1 (size=%2)").arg(path).arg(sz));
            continue;
        }

        // Attempt removal
        appendLog(QString("Cleaner: removing temp file %1 (size=%2)").arg(path).arg(sz));
        if (QFile::remove(path)) {
            removedCount++;
            freedBytes += sz;
        } else {
            appendLog(QString("Cleaner: failed to remove %1").arg(path));
        }
    }

    // Tidy up a few other well-known filenames (backwards compatibility)
    const QString aboutLog = tmpDir.filePath(QStringLiteral("lsv-about-links.log"));
    QFileInfo aboutFi(aboutLog);
    if (aboutFi.exists() && ownedByCurrentUser(aboutLog)) {
        if (aboutFi.size() < PRESERVE_IF_LARGER_THAN && aboutFi.lastModified().secsTo(now) > MIN_AGE_SECS) {
            appendLog(QString("Cleaner: removing small about-links log %1").arg(aboutLog));
            if (QFile::remove(aboutLog)) {
                removedCount++;
                freedBytes += aboutFi.size();
            }
        } else {
            appendLog(QString("Cleaner: preserving about-links log %1 (size=%2 age=%3s)").arg(aboutLog).arg(aboutFi.size()).arg(aboutFi.lastModified().secsTo(now)));
        }
    }

    appendLog(QString("Cleaner: finished. Removed %1 items, freed %2 bytes").arg(QString::number(removedCount)).arg(QString::number(freedBytes)));
}

// Subclass QMainWindow to intercept closeEvent and show a cleaning dialog
class CleaningMainWindow : public QMainWindow
{
public:
    using QMainWindow::QMainWindow;

protected:
    void closeEvent(QCloseEvent *event) override
    {
        // Start cleanup in background. Only show the modal dialog if cleanup
        // takes longer than a short threshold (500 ms) to avoid a UI blink
        // on fast systems; on slower machines the dialog will appear and
        // remain until cleanup completes.
        QFuture<void> fut = QtConcurrent::run(performCleanup);
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
        watcher->setFuture(fut);

        // Prepare dialog but do not show it immediately.
        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle(QStringLiteral("Cleaning up"));
        QVBoxLayout *lay = new QVBoxLayout(dlg);
        QLabel *lbl = new QLabel(QStringLiteral("Cleaning up temporary files..."), dlg);
        lay->addWidget(lbl);
        QProgressBar *pb = new QProgressBar(dlg);
        pb->setRange(0, 0); // indeterminate
        lay->addWidget(pb);
        dlg->setModal(true);
        dlg->setMinimumWidth(360);

        bool finished = false;
        QEventLoop loop;
        QElapsedTimer shownTimer;

        // When cleanup finishes, mark finished and close the dialog if shown,
        // then quit the nested event loop so closeEvent can proceed. If the
        // dialog was shown, ensure it remains visible for at least
        // MIN_DISPLAY_MS milliseconds before closing to avoid a too-quick blink.
        const int MIN_DISPLAY_MS = 1000; // keep dialog visible at least 1s if shown
        QObject::connect(watcher, &QFutureWatcher<void>::finished, this, [dlg, &finished, &loop, &shownTimer]() {
            finished = true;
            if (dlg->isVisible()) {
                // If shownTimer wasn't started for some reason, close immediately.
                if (!shownTimer.isValid()) {
                    dlg->accept();
                    if (loop.isRunning()) loop.quit();
                    return;
                }
                qint64 elapsed = shownTimer.elapsed();
                if (elapsed < MIN_DISPLAY_MS) {
                    int remaining = int(MIN_DISPLAY_MS - elapsed);
                    QTimer::singleShot(remaining, dlg, [dlg, &loop]() {
                        if (dlg->isVisible()) dlg->accept();
                        if (loop.isRunning()) loop.quit();
                    });
                    return;
                } else {
                    dlg->accept();
                    if (loop.isRunning()) loop.quit();
                    return;
                }
            }
            if (loop.isRunning()) loop.quit();
        });

        // After threshold ms, show the dialog only if cleanup still running.
        const int SHOW_DELAY_MS = 500;
        QTimer::singleShot(SHOW_DELAY_MS, this, [dlg, &finished, &loop, &shownTimer]() {
            if (finished) return; // already done, don't flash dialog
            // Show dialog and start a nested loop that will quit when the
            // watcher finishes (connected above).
            dlg->show();
            shownTimer.start();
            QObject::connect(dlg, &QDialog::finished, &loop, &QEventLoop::quit);
            // The nested loop will be executed in closeEvent below.
        });

        // If cleanup already finished by the time we reach here, skip the loop.
        if (!finished) loop.exec();

        // Cleanup dialog closed (if shown). Ensure dialog is deleted.
        if (dlg->isVisible()) dlg->accept();
        dlg->deleteLater();
        // watcher will be deleted with this as parent

        // Let the normal close proceed
        event->accept();
    }
};

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(QObject* parent = nullptr) : QObject(parent) {}

    void createAllTabs()
    {
        for (int i = 0; i < TAB_CONFIGS.size(); ++i) {
            const TabConfig& config = TAB_CONFIGS[i];
            qDebug() << "TabManager: Creating tab" << i << ":" << config.name;
            appendLog(QString("TabManager: Creating tab %1 : %2 (command: %3)").arg(QString::number(i), config.name, config.command));
            
            QWidget* tabWidget = createTab(config);
            if (tabWidget) {
                m_tabWidget->addTab(tabWidget, config.name);
                qDebug() << "TabManager: Successfully added tab:" << config.name;
            } else {
                qDebug() << "TabManager: Failed to create tab:" << config.name;
            }
        }
    }

    void setTabWidget(MultiRowTabWidget* tabWidget)
    {
        m_tabWidget = tabWidget;
    }

signals:
    void tabLoadingStarted(const QString& tabName);
    void tabLoadingFinished(const QString& tabName);

private slots:
    void onTabLoadingStarted()
    {
        TabWidgetBase* tab = qobject_cast<TabWidgetBase*>(sender());
        if (tab) {
            emit tabLoadingStarted(tab->getTabName());
        }
    }

    void onTabLoadingFinished()
    {
        TabWidgetBase* tab = qobject_cast<TabWidgetBase*>(sender());
        if (tab) {
            emit tabLoadingFinished(tab->getTabName());
        }
    }

private:
    QWidget* createTab(const TabConfig& config)
    {
        QWidget* tabWidget = nullptr;
        
        if (config.name == "Summary") {
            SummaryTab* summaryTab = new SummaryTab();
            connect(summaryTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(summaryTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = summaryTab;
        }
        else if (config.name == "Memory") {
            tabWidget = new MemoryTab();
        }
        else if (config.name == "CPU") {
            CPUTab* cpuTab = new CPUTab();
            tabWidget = cpuTab;
        }
        else if (config.name == "OS") {
            appendLog(QString("TabManager: Instantiating OSTab with command: %1").arg(config.command));
            OSTab* osTab = new OSTab("OS", "lsb_release -a", true, "", nullptr);
            appendLog("TabManager: OSTab constructed");
            connect(osTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(osTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = osTab;
        }
        else if (config.name == "Audio") {
            AudioTab* audioTab = new AudioTab();
            connect(audioTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(audioTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = audioTab;
        }
        else if (config.name == "Desktop") {
            WindowingTab* windowingTab = new WindowingTab();
            connect(windowingTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(windowingTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = windowingTab;
        }
        else if (config.name == "Graphics gard") {
            GraphicsTab* graphicsTab = new GraphicsTab();
            connect(graphicsTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(graphicsTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = graphicsTab;
        }
        else if (config.name == "Screen") {
            ScreenTab* screenTab = new ScreenTab();
            connect(screenTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(screenTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = screenTab;
        }
        else if (config.name == "Ports") {
            PortsTab* portsTab = new PortsTab();
            connect(portsTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(portsTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = portsTab;
        }
        else if (config.name == "Peripherals") {
            PeripheralsTab* peripheralsTab = new PeripheralsTab();
            connect(peripheralsTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(peripheralsTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = peripheralsTab;
        }
        else if (config.name == "Motherboard") {
            MotherboardTab* motherboardTab = new MotherboardTab();
            connect(motherboardTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(motherboardTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = motherboardTab;
        }
        else if (config.name == "Disk") {
            StorageTab* storageTab = new StorageTab();
            connect(storageTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(storageTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = storageTab;
        }
        else if (config.name == "PC Info") {
            PCTab* pcTab = new PCTab();
            connect(pcTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(pcTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = pcTab;
        }
            else if (config.name == "About") {
                AboutTab* aboutTab = new AboutTab();
                connect(aboutTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
                connect(aboutTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
                tabWidget = aboutTab;
            }
            else if (config.name == "Network") {
                NetworkTab* netTab = new NetworkTab();
                connect(netTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
                connect(netTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
                tabWidget = netTab;
            }
        else {
            GenericTab* genericTab = new GenericTab(config.name, config.command, true, config.command);
            connect(genericTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(genericTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = genericTab;
        }
        
        return tabWidget;
    }

    MultiRowTabWidget* m_tabWidget = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Central version constant
#include "version.h"

    // Set application properties
    app.setApplicationName("Linux System Viewer");
    app.setApplicationVersion(LSVVersionQString());
    app.setOrganizationName("Linux System Viewer");

    // Set application icon from resource
    QIcon appIcon(":/lsv.png");
    app.setWindowIcon(appIcon);

    // Install Qt message handler so all qDebug/qWarning/etc go to the
    // appendLog file instead of printing to the console.
    qInstallMessageHandler(lsvQtMessageHandler);

    qDebug() << "Application starting..."; // will be routed to appendLog
    appendLog(QString("Application starting. CWD: %1, log-file: %2").arg(QDir::currentPath(), QDir::currentPath()+"/lsv-cli.log"));

    // Auto-elevation: always relaunch via a terminal sudo prompt and exit the
    // unprivileged instance. This ensures the user always authenticates in a
    // terminal window with a clear custom message and the GUI runs as root.
    if (geteuid() != 0 && qgetenv("LSV_ELEVATED").isEmpty()) {
        // Cleanup old temp files to avoid clutter.
        QDir tmpDir(QDir::tempPath());
        QDateTime now = QDateTime::currentDateTime();
        const int MAX_AGE_SECS = 60 * 60; // 1 hour
        QStringList stalePatterns = {"lsv-elevated-*"};
        for (const QString &pat : stalePatterns) {
            QFileInfoList entries = tmpDir.entryInfoList(QStringList(pat), QDir::Files);
            for (const QFileInfo &fi : entries) {
                if (fi.lastModified().secsTo(now) > MAX_AGE_SECS) QFile::remove(fi.absoluteFilePath());
            }
        }

        QString exe = QCoreApplication::applicationFilePath();
        QString preCopiedExe;
        // If running from an AppImage mount, try to pre-copy the binary so
        // root can execute it even if the FUSE mount becomes inaccessible.
        if (exe.contains("/tmp/.mount_")) {
            preCopiedExe = QDir::tempPath() + QDir::separator() + QString("lsv-elevated-%1").arg(QCoreApplication::applicationPid());
            QFile::remove(preCopiedExe);
            bool copied = QFile::copy(exe, preCopiedExe);
            if (!copied) {
                QFile in("/proc/self/exe");
                if (in.open(QIODevice::ReadOnly)) {
                    QFile out(preCopiedExe);
                    if (out.open(QIODevice::WriteOnly)) {
                        const qint64 bufSize = 32768;
                        while (!in.atEnd()) out.write(in.read(bufSize));
                        out.close();
                        copied = true;
                    }
                    in.close();
                }
            }
            if (!copied) {
                QProcess cpProc;
                cpProc.start("sh", QStringList() << "-c" << QString("cat /proc/self/exe > '%1' && chmod 0755 '%1'").arg(preCopiedExe));
                if (cpProc.waitForFinished(5000)) {
                    if (QFile::exists(preCopiedExe) && QFile(preCopiedExe).size() > 0) copied = true;
                }
            }
            if (copied) {
                QFile::setPermissions(preCopiedExe, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner
                                                   | QFile::ExeGroup | QFile::ReadGroup
                                                   | QFile::ExeOther | QFile::ReadOther);
                appendLog(QString("Auto-elevation: Copied mounted exe to %1").arg(preCopiedExe));
            } else {
                appendLog(QString("Auto-elevation: Failed to copy mounted exe %1 -> %2").arg(exe, preCopiedExe));
                preCopiedExe.clear();
            }
        }

        QString targetExe = preCopiedExe.isEmpty() ? exe : preCopiedExe;

        // Find a terminal emulator to run sudo
        QStringList terms = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "mate-terminal", "lxterminal", "xterm", "alacritty", "terminator"};
        QString termPath;
        for (const QString &t : terms) {
            QString p = QStandardPaths::findExecutable(t);
            if (!p.isEmpty()) { termPath = p; break; }
        }
        if (termPath.isEmpty()) {
            // Can't prompt in a terminal; inform the user and exit.
            appendLog("Auto-elevation: No terminal emulator found to prompt for password. Exiting.");
            QMessageBox::critical(nullptr, "Cannot elevate", "No terminal emulator found to prompt for a password.\nPlease run the application as root.");
            return 0;
        }

        // Build the sudo command that authenticates and then starts the GUI
        // as a detached process so the terminal can close after auth.
        QString sudoPrompt = "Please enter password to run Linux System Viewer as root";
        QString escTarget = targetExe;
        escTarget.replace('\'', "'" "'" "'");
        QString inner = QString("setsid '%1' > /dev/null 2>&1 &").arg(escTarget);
        // Create a temporary wrapper script to run sudo. This reduces quoting
        // issues when passing complex commands to terminal emulators.
        QString wrapperPath = QDir::tempPath() + QDir::separator() + QString("lsv-sudo-%1.sh").arg(getpid());
        QFile wf(wrapperPath);
        if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream ts(&wf);
            ts << "#!/bin/bash\n";
            ts << "echo 'LSV wrapper starting at ' $(date) > /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "echo 'Running sudo to start LSV as root' >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            // Prompt and allow up to 3 attempts. Use read -s so Enter works
            // normally and let sudo validate. On success exit; after 3 bad
            // attempts give up.
            QString promptEsc = sudoPrompt;
            promptEsc.replace('\'' , "'" "'" "'");
            ts << "printf '\\033]0;Linux System Viewer\\007'\n";
            ts << "attempts=0\n";
            ts << "while [ $attempts -lt 3 ]; do\n";
            ts << "  attempts=$((attempts+1))\n";
            ts << "  printf '%s: ' '" << promptEsc << "'\n";
            ts << "  read -s PASS\n";
            ts << "  echo\n";
            QString innerEsc = inner;
            innerEsc.replace('"', "\\\"");
            ts << "  printf '%s\\n' \"$PASS\" | sudo -S -p '' sh -c \"" << innerEsc << "\"\n";
            ts << "  rc=$?\n";
            ts << "  echo 'sudo finished with exitcode:' $rc >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "  if [ $rc -eq 0 ]; then exit 0; fi\n";
            ts << "  echo 'Authentication failed ('$attempts'/3)' >&2\n";
            ts << "done\n";
            ts << "echo 'Giving up after 3 failed attempts' >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "exit 1\n";
            wf.close();
            QFile::setPermissions(wrapperPath, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
        } else {
            appendLog(QString("Auto-elevation: Failed to write wrapper script %1").arg(wrapperPath));
        }

        QStringList args;
        // Try to request a small terminal height (3 rows) where supported.
        QString base = QFileInfo(termPath).fileName();
        QString geom = "80x3"; // width x height
        if (base.contains("gnome-terminal")) {
            args << QString("--geometry=%1").arg(geom) << "--" << "bash" << "-c" << QString("bash '%1'").arg(wrapperPath);
        } else if (base.contains("konsole")) {
            args << QString("--geometry") << geom << "-e" << "bash" << "-c" << QString("bash '%1'").arg(wrapperPath);
        } else if (base.contains("xterm") || base.contains("x-terminal-emulator")) {
            args << "-geometry" << geom << "-e" << QString("bash -c '%1'").arg(wrapperPath);
        } else {
            // Generic terminals: attempt -e without geometry
            args << "-e" << QString("bash -c '%1'").arg(wrapperPath);
        }

        bool ok = QProcess::startDetached(termPath, args);
        if (ok) {
            appendLog(QString("Auto-elevation: Launched terminal '%1' to prompt for sudo (wrapper: %2)").arg(termPath, wrapperPath));
        } else {
            appendLog(QString("Auto-elevation: Failed to launch terminal '%1' for sudo prompt (wrapper: %2)").arg(termPath, wrapperPath));
            QMessageBox::critical(nullptr, "Elevation failed", "Failed to start a terminal to request sudo password. Please run the application as root.");
        }

        // Exit the unprivileged instance immediately; the elevated GUI will
        // be started from the terminal after authentication.
        return 0;
    }

    // Create main window
    CleaningMainWindow mainWindow;
    mainWindow.setWindowTitle(QStringLiteral("Linux System Viewer V. %1").arg(LSVVersionQString()));
    mainWindow.setWindowIcon(appIcon);

    // Set fixed initial size (850x480) regardless of DPI
    const int INITIAL_WIDTH = 850;
    const int INITIAL_HEIGHT = 480;
    
    mainWindow.resize(INITIAL_WIDTH, INITIAL_HEIGHT);
    mainWindow.setMinimumSize(600, 300);

    qDebug() << "Window size set to:" << INITIAL_WIDTH << "x" << INITIAL_HEIGHT;

    // Create central widget
    QWidget* centralWidget = new QWidget();
    mainWindow.setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Create elevation status label (admin shield)
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(QStringLiteral("Linux System Viewer V. %1").arg(LSVVersionQString()));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    titleLabel->setFont(titleFont);

    // About button (colored badge): blue for normal user, green for superuser
    QToolButton* aboutBtn = new QToolButton;
    QColor bg = (geteuid() == 0) ? QColor("#2ecc71") : QColor("#3498db");
    QPixmap badge = makeBadgePixmap(bg, 20);
    aboutBtn->setIcon(QIcon(badge));
    aboutBtn->setIconSize(QSize(20,20));
    aboutBtn->setAutoRaise(true);
    aboutBtn->setToolTip("About Linux System Viewer");
    QObject::connect(aboutBtn, &QAbstractButton::clicked, [&app]() {
        // Show AboutTab as a top-level window/dialog
        AboutTab* about = new AboutTab();
        about->setAttribute(Qt::WA_DeleteOnClose);
        about->setWindowModality(Qt::ApplicationModal);
        about->show();
        about->raise();
        about->activateWindow();
    });

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(aboutBtn);
    mainLayout->addLayout(titleLayout);

    // Create tab widget
    MultiRowTabWidget* tabWidget = new MultiRowTabWidget();
    mainLayout->addWidget(tabWidget);

    // Install global Ctrl+W / close handler so Ctrl+W shows a quit dialog
    // and window-close events can be intercepted. The handler is parented
    // to the main window so it lives as long as the window does.
    CtrlWHandler* ctrlHandler = new CtrlWHandler(&mainWindow, &mainWindow);
    Q_UNUSED(ctrlHandler);

    // Create tab manager and populate tabs
    TabManager tabManager;
    tabManager.setTabWidget(tabWidget);

    // Status connections
    QObject::connect(&tabManager, &TabManager::tabLoadingStarted, [](const QString& tabName) {
        qDebug() << "Loading started for tab:" << tabName;
    });

    QObject::connect(&tabManager, &TabManager::tabLoadingFinished, [](const QString& tabName) {
        qDebug() << "Loading finished for tab:" << tabName;
    });

    // Show main window first so the UI appears even if tab construction takes time.
    mainWindow.show();
    qDebug() << "Application window shown, scheduling tab creation...";

    // Defer heavy tab creation to the event loop so the window can render immediately.
    QTimer::singleShot(0, [&tabManager]() {
        qDebug() << "Creating tabs...";
        tabManager.createAllTabs();
        qDebug() << "All tabs created successfully";
    });

    qDebug() << "Entering event loop...";
    return app.exec();
}

#include "lsv.moc"
#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <sys/stat.h>
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

// Forward-declare appendLog from log_helper.h
#include "log_helper.h"

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
#include "tabs_config.h"
#include "pc_tab.h"
#include "memory_tab.h"
#include "log_helper.h"

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

    // Set application properties
    app.setApplicationName("Linux System Viewer");
    app.setApplicationVersion("0.6.0");
    app.setOrganizationName("LSV");

    // Set application icon from resource
    QIcon appIcon(":/lsv.png");
    app.setWindowIcon(appIcon);

    // Install Qt message handler so all qDebug/qWarning/etc go to the
    // appendLog file instead of printing to the console.
    qInstallMessageHandler(lsvQtMessageHandler);

    qDebug() << "Application starting..."; // will be routed to appendLog
    appendLog(QString("Application starting. CWD: %1, log-file: %2").arg(QDir::currentPath(), QDir::currentPath()+"/lsv-cli.log"));

    // Auto-elevation: if not running as root, try to relaunch via pkexec using
    // a temporary wrapper script that preserves necessary environment
    // variables (DISPLAY, XAUTHORITY, DBUS, XDG_RUNTIME_DIR). This keeps the
    // app portable (no install) while still prompting for password.
    if (geteuid() != 0 && qgetenv("LSV_ELEVATED").isEmpty()) {
        // Cleanup old temp files to avoid clutter. Remove lsv-elevated-*
        // and lsv-relaunch-* files older than an hour.
        QDir tmpDir(QDir::tempPath());
        QDateTime now = QDateTime::currentDateTime();
        const int MAX_AGE_SECS = 60 * 60; // 1 hour
        QStringList stalePatterns = {"lsv-elevated-*", "lsv-relaunch-*.sh", "lsv-relaunch-*.log"};
        for (const QString &pat : stalePatterns) {
            QFileInfoList entries = tmpDir.entryInfoList(QStringList(pat), QDir::Files);
            for (const QFileInfo &fi : entries) {
                if (fi.lastModified().secsTo(now) > MAX_AGE_SECS) {
                    QFile::remove(fi.absoluteFilePath());
                }
            }
        }
        QString pkexecPath = QStandardPaths::findExecutable("pkexec");
        QString exe = QCoreApplication::applicationFilePath();
        QString tmpScript = QDir::tempPath() + QDir::separator() + QString("lsv-relaunch-%1.sh").arg(getpid());
        // If running from an AppImage mount, copy the executable now (as the
        // current user) into /tmp so the elevated pkexec child can execute it
        // even if the AppImage mount is removed when this process exits.
        QString preCopiedExe;
        if (exe.contains("/tmp/.mount_")) {
            preCopiedExe = QDir::tempPath() + QDir::separator() + QString("lsv-elevated-%1").arg(QCoreApplication::applicationPid());
            QFile::remove(preCopiedExe);
            // First try a direct copy of the mounted path. If that fails
            // (common on some FUSE-mounted AppImage setups where root cannot
            // access the user's mount), fall back to copying the running
            // process image via /proc/self/exe which the current user can
            // read.
            bool copied = QFile::copy(exe, preCopiedExe);
            // Diagnostic: record whether the source exe is visible to the
            // unprivileged process and its size. This helps debug cases
            // where the FUSE-mounted path isn't readable to either the
            // unprivileged or the elevated process.
            QFileInfo srcInfo(exe);
            if (srcInfo.exists()) {
                appendLog(QString("Auto-elevation: source exe exists: %1 size=%2").arg(exe).arg(QString::number(srcInfo.size())));
            } else {
                appendLog(QString("Auto-elevation: source exe does NOT exist (from user's view): %1").arg(exe));
            }
            if (!copied) {
                appendLog(QString("Auto-elevation: direct copy failed, trying /proc/self/exe fallback"));
                QFile in("/proc/self/exe");
                if (in.open(QIODevice::ReadOnly)) {
                    QFile out(preCopiedExe);
                    if (out.open(QIODevice::WriteOnly)) {
                        const qint64 bufSize = 32768;
                        QByteArray buf;
                        while (!in.atEnd()) {
                            buf = in.read(bufSize);
                            out.write(buf);
                        }
                        out.close();
                        copied = true;
                    }
                    in.close();
                } else {
                    appendLog(QString("Auto-elevation: failed to open /proc/self/exe for fallback copy"));
                }
            }
            
            // If fallback read also failed, try a shell-level copy using cat
            // which sometimes behaves better for special files.
            if (!copied) {
                appendLog(QString("Auto-elevation: attempting shell-level copy via cat"));
                QProcess cpProc;
                QString cmd = QString("sh -c 'cat /proc/self/exe > %1 && chmod 0755 %1'").arg(preCopiedExe);
                cpProc.start("sh", QStringList() << "-c" << QString("cat /proc/self/exe > '%1' && chmod 0755 '%1'").arg(preCopiedExe));
                bool started = cpProc.waitForStarted(2000);
                if (started) {
                    cpProc.waitForFinished(5000);
                    if (QFile::exists(preCopiedExe) && QFile(preCopiedExe).size() > 0) {
                        copied = true;
                        appendLog(QString("Auto-elevation: shell-level copy succeeded to %1").arg(preCopiedExe));
                    } else {
                        appendLog(QString("Auto-elevation: shell-level copy failed"));
                    }
                } else {
                    appendLog(QString("Auto-elevation: could not start shell copy process"));
                }
            }
            if (copied) {
                QFile::setPermissions(preCopiedExe, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner
                                                   | QFile::ExeGroup | QFile::ReadGroup
                                                   | QFile::ExeOther | QFile::ReadOther);
                appendLog(QString("Auto-elevation: Copied mounted exe to %1").arg(preCopiedExe));
                    // Report the size of the pre-copied file for debugging.
                    QFileInfo preInfo(preCopiedExe);
                    appendLog(QString("Auto-elevation: pre-copied file size: %1").arg(QString::number(preInfo.size())));
            } else {
                appendLog(QString("Auto-elevation: Failed to copy mounted exe %1 -> %2").arg(exe, preCopiedExe));
                preCopiedExe.clear();
            }
        }
        // If an installed setuid helper is available, prefer it because
        // it avoids relying on polkit agents and can be more reliable
        // when launched from a file manager. The helper must be installed
        // by a system administrator (chown root:root && chmod 4755).
        QString helperExec = QStandardPaths::findExecutable("lsv-elevate");
        if (!helperExec.isEmpty()) {
            appendLog(QString("Auto-elevation: found helper %1, launching helper").arg(helperExec));
            // If we pre-copied the exe, prefer that path so helper doesn't
            // need to access the FUSE-mounted AppImage path.
            QString helperTarget = preCopiedExe.isEmpty() ? exe : preCopiedExe;
            bool okh = QProcess::startDetached(helperExec, QStringList() << helperTarget);
            if (okh) {
                appendLog("Auto-elevation: Relaunched with lsv-elevate helper");
                QObject::connect(&app, &QCoreApplication::aboutToQuit, [tmpScript, preCopiedExe]() {
                    if (!tmpScript.isEmpty()) QFile::remove(tmpScript);
                    if (!preCopiedExe.isEmpty()) QFile::remove(preCopiedExe);
                });

                // Start the same watchdog as below to detect elevated instance
                QString watchExe = preCopiedExe.isEmpty() ? exe : preCopiedExe;
                QTimer* watchTimer = new QTimer();
                watchTimer->setInterval(500);
                int maxChecks = 20;
                int* checks = new int(0);
                QObject::connect(watchTimer, &QTimer::timeout, [watchTimer, watchExe, checks, maxChecks]() {
                    (*checks)++;
                    QProcess p; p.start("ps", QStringList() << "-eo" << "pid,user,cmd"); p.waitForFinished(1000);
                    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
                    bool found = false;
                    QString watchBase = QFileInfo(watchExe).fileName();
                    for (const QString& line : out.split('\n')) {
                        if (line.isEmpty()) continue;
                        if (line.contains(" root ") && (line.contains(watchExe) || (!watchBase.isEmpty() && line.contains(watchBase)))) {
                            found = true; break;
                        }
                    }
                    if (found || *checks >= maxChecks) {
                        watchTimer->stop(); watchTimer->deleteLater(); delete checks;
                        if (found) qApp->quit();
                    }
                });
                watchTimer->start();
                // We initiated elevation via helper; don't fall through to pkexec.
            } else {
                appendLog("Auto-elevation: failed to start lsv-elevate helper, falling back to pkexec if available");
            }
        }

        if (!pkexecPath.isEmpty()) {
            QFile f(tmpScript);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream ts(&f);
                ts << "#!/bin/sh\n";
                // Export the elevation marker so child won't re-run this branch
                ts << "export LSV_ELEVATED=1\n";
                QByteArray display = qgetenv("DISPLAY");
                if (!display.isEmpty()) ts << "export DISPLAY='" << QString::fromLocal8Bit(display).replace('\'' , "'" "'" "'") << "'\n";
                QByteArray xauth = qgetenv("XAUTHORITY");
                if (!xauth.isEmpty()) ts << "export XAUTHORITY='" << QString::fromLocal8Bit(xauth).replace('\'' , "'" "'" "'") << "'\n";
                QByteArray dbus = qgetenv("DBUS_SESSION_BUS_ADDRESS");
                if (!dbus.isEmpty()) ts << "export DBUS_SESSION_BUS_ADDRESS='" << QString::fromLocal8Bit(dbus).replace('\'' , "'" "'" "'") << "'\n";
                QByteArray xdg = qgetenv("XDG_RUNTIME_DIR");
                if (!xdg.isEmpty()) ts << "export XDG_RUNTIME_DIR='" << QString::fromLocal8Bit(xdg).replace('\'' , "'" "'" "'") << "'\n";
                // If the application is running from an AppImage-mounted location
                // (path contains "/tmp/.mount_"), pkexec running as root may be
                // unable to execute the mounted binary directly. Prefer to use
                // a pre-copied executable (copied earlier as the unprivileged
                // user) if available; otherwise copy via mktemp in the wrapper
                // and exec that copy.
                if (exe.contains("/tmp/.mount_")) {
                    if (!preCopiedExe.isEmpty()) {
                        QString preEsc = preCopiedExe;
                        preEsc.replace('\'', "'" "'" "'");
                        ts << "LOG=/tmp/lsv-relaunch-" << getpid() << ".log\n";
                        ts << "echo \"=== lsv-relaunch wrapper start $(date) PID=$$\" > \"$LOG\"\n";
                        ts << "echo \"Using pre-copied exec: '" << preEsc << "'\" >> \"$LOG\"\n";
                        ts << "ls -l '" << preEsc << "' >> \"$LOG\" 2>&1 || true\n";
                        ts << "exec '" << preEsc << "' >> \"$LOG\" 2>&1 || true\n";
                    } else {
                        // Use mktemp to create a unique temporary filename for the
                        // elevated copy. This avoids permission problems when a
                        // deterministically-named file already exists and is not
                        // writable by the calling user.
                        QString exeEsc = exe;
                        exeEsc.replace('\'', "'" "'" "'");
                        // More verbose wrapper: log steps to /tmp/lsv-relaunch-<pid>.log
                        ts << "LOG=/tmp/lsv-relaunch-" << getpid() << ".log\n";
                        ts << "echo \"=== lsv-relaunch wrapper start $(date) PID=$$\" > \"$LOG\"\n";
                        ts << "echo \"exe: '" << exeEsc << "'\" >> \"$LOG\"\n";
                        ts << "ls -l '" << exeEsc << "' >> \"$LOG\" 2>&1 || true\n";
                        ts << "stat '" << exeEsc << "' >> \"$LOG\" 2>&1 || true\n";
                        // Create a unique temporary filename under /tmp
                        ts << "TMPEXEC=$(mktemp /tmp/lsv-elevated-XXXXXX)\n";
                        ts << "echo \"Using temp exec: $TMPEXEC\" >> \"$LOG\" 2>&1 || true\n";
                        // Try cp, fall back to cat if cp fails (some fused mounts behave oddly)
                        ts << "cp '" << exeEsc << "' \"$TMPEXEC\" >> \"$LOG\" 2>&1 || (cat '" << exeEsc << "' > \"$TMPEXEC\" 2>> \"$LOG\" || true)\n";
                        ts << "chmod 0755 \"$TMPEXEC\" >> \"$LOG\" 2>&1 || true\n";
                        ts << "echo \"After copy: \" >> \"$LOG\"; ls -l \"$TMPEXEC\" >> \"$LOG\" 2>&1 || true\n";
                        ts << "exec \"$TMPEXEC\" >> \"$LOG\" 2>&1 || (echo \"exec $TMPEXEC failed, trying original\" >> \"$LOG\"; exec '" << exeEsc << "')\n";
                    }
                } else {
                    ts << "exec '" << exe.replace('\'' , "'" "'" "'") << "'\n";
                }
                f.close();
                QFile::setPermissions(tmpScript, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);

                bool launched = false;
                // Prefer pkexec when a polkit GUI agent is present. If no GUI
                // agent is detected, fall back to launching a terminal emulator
                // that runs sudo so the user can authenticate in a terminal.
                if (polkitAgentRunning()) {
                    bool ok = QProcess::startDetached(pkexecPath, QStringList() << "/bin/sh" << tmpScript);
                    if (ok) {
                        appendLog("Auto-elevation: Relaunched with pkexec");
                        launched = true;
                    } else {
                        appendLog("Auto-elevation: pkexec startDetached failed");
                    }
                } else {
                    appendLog("Auto-elevation: no polkit GUI agent detected, attempting terminal sudo fallback");
                    // Search for a terminal emulator to run the sudo fallback
                    QStringList terms = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "mate-terminal", "lxterminal", "xterm", "alacritty", "terminator"};
                    QString termPath;
                    for (const QString &t : terms) {
                        QString p = QStandardPaths::findExecutable(t);
                        if (!p.isEmpty()) { termPath = p; break; }
                    }
                    if (!termPath.isEmpty()) {
                        QStringList args;
                        // Run the relaunch script via sudo; do NOT wait for an
                        // extra keypress so the terminal closes as soon as the
                        // command finishes (user requested immediate close).
                        QString cmd = QString("sudo -E /bin/sh '%1'").arg(tmpScript);
                        if (termPath.endsWith("gnome-terminal") || termPath.endsWith("gnome-terminal.real")) {
                            args << "--" << "bash" << "-c" << cmd;
                        } else if (termPath.endsWith("konsole")) {
                            args << "-e" << "bash" << "-c" << cmd;
                        } else {
                            // Most terminals accept -e
                            args << "-e" << QString("sh -c \"%1\"").arg(cmd);
                        }
                        bool okTerm = QProcess::startDetached(termPath, args);
                        if (okTerm) {
                            appendLog(QString("Auto-elevation: Launched terminal '%1' for sudo fallback").arg(termPath));
                            launched = true;
                        } else {
                            appendLog(QString("Auto-elevation: Failed to launch terminal '%1' for sudo fallback").arg(termPath));
                        }
                    } else {
                        appendLog("Auto-elevation: No terminal emulator found for sudo fallback");
                    }
                }
                if (launched) {
                    // Schedule cleanup of the temporary script and any pre-copied exe on exit.
                    QObject::connect(&app, &QCoreApplication::aboutToQuit, [tmpScript, preCopiedExe]() {
                        if (!tmpScript.isEmpty()) QFile::remove(tmpScript);
                        if (!preCopiedExe.isEmpty()) QFile::remove(preCopiedExe);
                    });

                    // Start a short watchdog: if an elevated LSV process appears
                    // (owned by root and matching the expected executable path),
                    // quit the non-elevated instance so the elevated GUI becomes
                    // the only window.
                    QString watchExe = preCopiedExe.isEmpty() ? exe : preCopiedExe;
                    QTimer* watchTimer = new QTimer();
                    watchTimer->setInterval(500); // check twice per second
                    int maxChecks = 20; // 10 seconds
                    int* checks = new int(0);
                    QObject::connect(watchTimer, &QTimer::timeout, [watchTimer, watchExe, checks, maxChecks, pkexecPath, tmpScript, preCopiedExe]() {
                        (*checks)++;
                        // run ps to find root-owned process running watchExe
                        QProcess p;
                        p.start("ps", QStringList() << "-eo" << "pid,user,cmd");
                        p.waitForFinished(1000);
                        QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
                        bool found = false;
                        QString watchBase = QFileInfo(watchExe).fileName();
                        for (const QString& line : out.split('\n')) {
                            if (line.isEmpty()) continue;
                            // Match either the full path or the executable basename
                            if (line.contains(" root ") && (line.contains(watchExe) || (!watchBase.isEmpty() && line.contains(watchBase)))) {
                                // Write to the wrapper log so the user can see detection
                                QFile logf(QString("/tmp/lsv-relaunch-%1.log").arg(getpid()));
                                if (logf.open(QIODevice::Append | QIODevice::Text)) {
                                    QTextStream lts(&logf);
                                    lts << "Detected elevated process line: " << line << "\n";
                                    logf.close();
                                }
                                found = true;
                                break;
                            }
                        }
                        if (found || *checks >= maxChecks) {
                            watchTimer->stop();
                            watchTimer->deleteLater();
                            delete checks;
                            if (found) {
                                // Elevated instance started; quit non-elevated app.
                                qApp->quit();
                            } else {
                                // Watchdog timed out. Inform the user and offer actions.
                                appendLog("Auto-elevation: watchdog timeout, elevated instance not detected");
                                QString logPath = QString("/tmp/lsv-relaunch-%1.log").arg(getpid());
                                QMessageBox msg;
                                msg.setWindowTitle("Elevation failed");
                                msg.setText("Elevation did not complete. The application will continue in unprivileged mode.\n\nChoose an action:");
                                QPushButton *retryBtn = msg.addButton("Retry Elevation", QMessageBox::AcceptRole);
                                QPushButton *helperBtn = msg.addButton("Use installed helper (lsv-elevate)", QMessageBox::ActionRole);
                                QPushButton *showLogBtn = msg.addButton("Show elevation log", QMessageBox::ActionRole);
                                QPushButton *cancelBtn = msg.addButton(QMessageBox::Cancel);
                                msg.exec();
                                if (msg.clickedButton() == retryBtn) {
                                    // Retry pkexec
                                    if (!pkexecPath.isEmpty()) {
                                        bool ok = QProcess::startDetached(pkexecPath, QStringList() << "/bin/sh" << tmpScript);
                                        if (ok) {
                                            appendLog("Auto-elevation: Retry requested - relaunched with pkexec");
                                            // Recreate watchdog: start a new timer and continue monitoring
                                            int* newChecks = new int(0);
                                            QTimer* newWatch = new QTimer();
                                            newWatch->setInterval(500);
                                            QObject::connect(newWatch, &QTimer::timeout, [newWatch, watchExe, newChecks, maxChecks, pkexecPath, tmpScript, preCopiedExe]() {
                                                (*newChecks)++;
                                                QProcess p2; p2.start("ps", QStringList() << "-eo" << "pid,user,cmd"); p2.waitForFinished(1000);
                                                QString out2 = QString::fromLocal8Bit(p2.readAllStandardOutput());
                                                QString watchBase2 = QFileInfo(watchExe).fileName();
                                                bool found2 = false;
                                                for (const QString& line2 : out2.split('\n')) {
                                                    if (line2.isEmpty()) continue;
                                                    if (line2.contains(" root ") && (line2.contains(watchExe) || (!watchBase2.isEmpty() && line2.contains(watchBase2)))) {
                                                        found2 = true; break;
                                                    }
                                                }
                                                if (found2 || *newChecks >= maxChecks) {
                                                    newWatch->stop(); newWatch->deleteLater(); delete newChecks;
                                                    if (found2) qApp->quit();
                                                }
                                            });
                                            newWatch->start();
                                        } else {
                                            appendLog("Auto-elevation: Retry pkexec startDetached failed");
                                        }
                                    }
                                } else if (msg.clickedButton() == helperBtn) {
                                    QString helper = QStandardPaths::findExecutable("lsv-elevate");
                                    if (!helper.isEmpty()) {
                                        bool ok = QProcess::startDetached(helper, QStringList() << QCoreApplication::applicationFilePath());
                                        if (ok) appendLog("Auto-elevation: launched lsv-elevate helper");
                                        else appendLog("Auto-elevation: failed to start lsv-elevate helper");
                                    } else {
                                        QMessageBox::information(nullptr, "Helper not found", "The lsv-elevate helper was not found. To enable one-click elevation from file manager, install the helper as root:\n\nsudo chown root:root /usr/local/bin/lsv-elevate && sudo chmod 4755 /usr/local/bin/lsv-elevate");
                                    }
                                } else if (msg.clickedButton() == showLogBtn) {
                                    if (!QProcess::startDetached("xdg-open", QStringList() << logPath)) {
                                        appendLog(QString("Auto-elevation: failed to open log %1").arg(logPath));
                                    }
                                } else {
                                    appendLog("Auto-elevation: user cancelled elevation options");
                                }
                            }
                        }
                    });
                    watchTimer->start();
                } else {
                    appendLog("Auto-elevation: pkexec startDetached failed");
                }
            } else {
                appendLog("Auto-elevation: failed to write temporary relaunch script");
            }
        } else {
            appendLog("Auto-elevation: pkexec not found, trying sudo fallback");
            QString sudoPath = QStandardPaths::findExecutable("sudo");
            if (!sudoPath.isEmpty()) {
                bool ok2 = QProcess::startDetached(sudoPath, QStringList() << "-E" << "/bin/sh" << tmpScript);
                if (ok2) {
                    appendLog("Auto-elevation: Relaunched with sudo -E");
                    QObject::connect(&app, &QCoreApplication::aboutToQuit, [tmpScript, preCopiedExe]() {
                        if (!tmpScript.isEmpty()) QFile::remove(tmpScript);
                        if (!preCopiedExe.isEmpty()) QFile::remove(preCopiedExe);
                    });
                } else {
                    appendLog("Auto-elevation: sudo startDetached failed");
                }
            } else {
                appendLog("Auto-elevation: sudo not found either");
            }
        }
    }

    // Create main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Linux System Viewer (LSV) v0.6.0");
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
    QLabel* titleLabel = new QLabel("Linux System Viewer (LSV) v0.6.0");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    titleLabel->setFont(titleFont);

    QLabel* adminLabel = new QLabel;
    if (geteuid() == 0) {
        // Use a less-conflicting icon than the red 'X' used by SP_MessageBoxCritical.
        // SP_DialogApplyButton is a check/approve icon which better indicates admin mode.
        QPixmap adminPixmap = app.style()->standardPixmap(QStyle::SP_DialogApplyButton);
        adminLabel->setPixmap(adminPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        adminLabel->setToolTip("Admin mode");
    }
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(adminLabel);
    mainLayout->addLayout(titleLayout);

    // Create tab widget
    MultiRowTabWidget* tabWidget = new MultiRowTabWidget();
    mainLayout->addWidget(tabWidget);

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
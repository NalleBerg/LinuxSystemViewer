#include <QtWidgets>
#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QTranslator>
#include <QSettings>
#include <QInputDialog>
#include <QVBoxLayout>
#include <sys/stat.h>
#include <pwd.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
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
        return QObject::tr("sudo apt update && sudo apt install policykit-1-gnome\n# then log out and back in (or run: /usr/lib/policykit-1-gnome/polkit-gnome-authentication-agent-1 &)");
    } else if (id.contains("fedora") || id.contains("rhel") || id.contains("centos")) {
        return QObject::tr("sudo dnf install polkit-gnome -y\n# then log out and back in (or run: /usr/libexec/polkit-gnome-authentication-agent-1 &)");
    } else if (id.contains("arch")) {
        return QObject::tr("sudo pacman -S polkit-gnome\n# then log out and back in (or run: /usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1 &)");
    }
    return QObject::tr("Please install a polkit authentication agent for your desktop (policykit-1-gnome, mate-polkit, polkit-kde) and log out/in.");
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

// ---------------------- i18n helpers ----------------------

// Translate tab names (explicit literals so lupdate picks them up).
static QString translateTabName(const QString &name)
{
    // Use QCoreApplication::translate with the same contexts used in the
    // translation TS file so the translator lookup succeeds. These
    // contexts correspond to the tab classes (SummaryTab, MemoryTab, ...).
    if (name == "Summary") return QCoreApplication::translate("SummaryTab", "Summary");
    if (name == "OS") return QCoreApplication::translate("OSTab", "OS");
    // The WindowingTab uses "Windowing environment" as the source string
    // in its translation context, so map the short config name "Desktop"
    // to that source so translations are found.
    if (name == "Desktop") return QCoreApplication::translate("WindowingTab", "Desktop");
    if (name == "Audio") return QCoreApplication::translate("AudioTab", "Audio");
    if (name == "Graphics gard") return QCoreApplication::translate("GraphicsTab", "Graphics");
    if (name == "Screen") return QCoreApplication::translate("ScreenTab", "Screen");
    if (name == "Ports") return QCoreApplication::translate("PortsTab", "Ports");
    if (name == "Peripherals") return QCoreApplication::translate("PeripheralsTab", "Peripherals");
    if (name == "Memory") return QCoreApplication::translate("MemoryTab", "Memory");
    if (name == "CPU") return QCoreApplication::translate("CPUTab", "CPU");
    if (name == "Motherboard") return QCoreApplication::translate("MotherboardTab", "Motherboard");
    if (name == "Disk") return QCoreApplication::translate("StorageTab", "Storage");
    if (name == "PC Info") return QCoreApplication::translate("PCTab", "PC Info");
    if (name == "About") return QCoreApplication::translate("AboutTab", "About");
    if (name == "Network") return QCoreApplication::translate("NetworkTab", "Network");
    // Fallback to the original name if no mapping exists
    return name;
}

static QMap<QString, QString> shippedLanguageDisplayNames()
{
    QMap<QString, QString> m;
    // Ship English (UK), German, Spanish, French, Norwegian Bokmål, Icelandic, Greek, Danish, Finnish, and Swedish in the UI list.
    // The repository and packaging contain these translators
    // and the application presents these languages to users.
    m.insert("en_GB", "English (UK)");
    m.insert("en", "English (UK)");
    m.insert("da", "Dansk");
    m.insert("de", "Deutsch");
    m.insert("el", "Ελληνικά");
    m.insert("es", "Español");
    m.insert("fi", "Suomi");
    m.insert("fr", "Français");
    m.insert("nb", "Norsk (Bokmål)");
    m.insert("is", "Íslenska");
    m.insert("sv", "Svenska");
    return m;
}

static QStringList discoverShippedLanguageCodes()
{
    QSet<QString> set;
    // Look in resource path :/i18n
    QDir resDir(":/i18n");
    if (resDir.exists()) {
        QStringList qms = resDir.entryList(QStringList() << "*.qm", QDir::Files);
        for (const QString &f : qms) {
            QString base = QFileInfo(f).completeBaseName();
            // Accept names like lsv_nb or nb
            QString code = base;
            if (code.startsWith("lsv_")) code = code.mid(4);
            set.insert(code);
        }
    }

    // Also look in local i18n/ directory next to the app binary
    QString appDir = QCoreApplication::applicationDirPath();
    QDir localDir(appDir + "/i18n");
    if (localDir.exists()) {
        QStringList qms = localDir.entryList(QStringList() << "*.qm", QDir::Files);
        for (const QString &f : qms) {
            QString base = QFileInfo(f).completeBaseName();
            QString code = base;
            if (code.startsWith("lsv_")) code = code.mid(4);
            set.insert(code);
        }
    }

    // If the shipped translators include any specific English locale
    // (en_*), prefer that and remove the plain "en" code so the UI
    // doesn't show duplicate English entries. Otherwise leave the
    // discovered codes as-is.
    bool hasEnVariant = false;
    for (const QString &c : set) {
        if (c.startsWith("en_")) { hasEnVariant = true; break; }
    }
    if (hasEnVariant) set.remove("en");
    // Exclude work-in-progress translations (Icelandic) from the
    // shipped/discoverable list so the published chooser remains
    // English+Norwegian only even if a developer has a local QM.
    set.remove("is");
    set.remove("is_IS");
    QStringList out = set.values();
    out.sort();
    return out;
}

static bool tryLoadTranslatorForCode(const QString &code, QApplication &app, QTranslator *translator)
{
    if (code == "en") return false; // English = default, no translator

    // Candidate paths/uris to try (resource and local filesystem)
    QStringList candidates;
    candidates << QString(":/i18n/lsv_%1.qm").arg(code);
    candidates << QString(":/i18n/%1.qm").arg(code);
    QString appDir = QCoreApplication::applicationDirPath();
    candidates << QString("%1/i18n/lsv_%2.qm").arg(appDir).arg(code);
    candidates << QString("%1/i18n/%2.qm").arg(appDir).arg(code);
    // No developer fallback here: only check resource and application
    // dir locations. Translations must be generated with lrelease and
    // embedded via CMake at configure time (recommended) or installed
    // to the application's i18n directory next to the binary.
    // Try each candidate
    for (const QString &p : candidates) {
        // For resource paths (":/..."), try loading directly. For
        // filesystem paths, ensure the file exists first. Avoid using
        // QResource::registerResource on resource URIs — it expects an
        // rcc file path and will fail for ":/" URIs.
        bool tryLoad = false;
        if (p.startsWith(":/")) {
            tryLoad = true;
        } else {
            tryLoad = QFile::exists(p);
        }
        if (!tryLoad) continue;
        if (translator->load(p)) {
            app.installTranslator(translator);
            appendLog(QString("i18n: Loaded translator for '%1' from %2").arg(code, p));
            return true;
        }
    }
    appendLog(QString("i18n: No translator found for '%1'").arg(code));
    return false;
}

// Normalize a requested language code to one of the available translator
// codes present in resources or the application i18n directory. This
// accepts codes like "is_IS" and will return "is" if only
// `lsv_is.qm` is available. Returns an empty string if nothing is
// available (or if code == "en").
static QString normalizeLanguageCodeToAvailable(const QString &code)
{
    if (code.isEmpty()) return QString();
    if (code == "en") return QString(); // English: no translator

    QStringList tryCodes;
    tryCodes << code;
    // If the code contains a region (lang_REGION), also try the base
    // language (lang) as a fallback.
    if (code.contains('_')) tryCodes << code.section('_', 0, 0);

    QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &c : tryCodes) {
        if (c.isEmpty()) continue;
        // resource candidates
        QString r1 = QString(":/i18n/lsv_%1.qm").arg(c);
        QString r2 = QString(":/i18n/%1.qm").arg(c);
        if (QFile::exists(r1) || QFile::exists(r2)) return c;
        // local file candidates next to the binary
        QString f1 = QString("%1/i18n/lsv_%2.qm").arg(appDir).arg(c);
        QString f2 = QString("%1/i18n/%2.qm").arg(appDir).arg(c);
        if (QFile::exists(f1) || QFile::exists(f2)) return c;
    }
    return QString();
}

// Path and helper for persistent language-choice "rc" file. We store a
// simple file containing the chosen language code in the user's config
// location (usually ~/.config/lsv_lang_rc). If the file exists its
// content will be used as the default language.
static QString langRcFilePath()
{
    // Primary RC now lives in the user's config directory per request.
    // Use ~/.config/LSV/lsv_lang.rc (QStandardPaths::ConfigLocation) and
    // ensure the path is returned; callers that write should create the
    // directory first.
    // When elevated (running as root), use the original user's config path
    // from LSV_USER_CONFIG environment variable so both elevated and
    // non-elevated instances share the same RC file.
    QString cfg;
    QString envUserConfig = QString::fromLocal8Bit(qgetenv("LSV_USER_CONFIG"));
    if (!envUserConfig.isEmpty()) {
        cfg = envUserConfig;
    } else {
        cfg = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    }
    if (cfg.isEmpty()) return QString();
    QDir d(cfg);
    QDir lsvDir(d.filePath("LSV"));
    return lsvDir.filePath("lsv_lang.rc");
}

// Return the primary RC path that lives next to the application binary.
// This intentionally does NOT fall back to the user config location: it's
// used to decide whether a portable `lsv_lang.rc` exists next to the
// binary and whether the language-chooser should be shown. The wrapper may
// set LSV_ORIG_APPDIR so an elevated instance can still locate the
// original application directory.
static QString langRcPrimaryPath()
{
    // The application MUST consult only the per-user config path.
    // No fallbacks are allowed — the rc file MUST live in
    // ~/.config/LSV/lsv_lang.rc to be considered.
    return langRcFilePath();
}

// Read only the primary (AppImage) RC file. Returns empty string if not
// present or unreadable. This intentionally does not consult the fallback
// config location.
static QString readLangRcPrimary()
{
    QString p = langRcPrimaryPath();
    if (p.isEmpty()) return QString();
    QFile f(p);
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString s = QString::fromLocal8Bit(f.readAll()).trimmed();
        f.close();
        return s;
    }
    return QString();
}

static QString readLangRc()
{
    // Read only the per-user config RC. No fallbacks allowed.
    QString user = langRcFilePath();
    if (user.isEmpty()) return QString();
    QFile fu(user);
    if (fu.exists() && fu.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString s = QString::fromLocal8Bit(fu.readAll()).trimmed();
        fu.close();
        return s;
    }
    return QString();
}

static bool writeLangRc(const QString &code)
{
    // Write the RC into the user's config directory (~/.config/LSV/lsv_lang.rc).
    QString user = langRcFilePath();
    if (user.isEmpty()) {
        appendLog(QString("i18n: cannot determine user lang rc path"));
        return false;
    }
    QFileInfo fi(user);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        if (!QDir().mkpath(dir.absolutePath())) {
            appendLog(QString("i18n: failed to create dir %1").arg(dir.absolutePath()));
            return false;
        }
    }
    QFile fa(user);
    if (fa.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream ts(&fa);
        ts << code << '\n';
        fa.close();
        appendLog(QString("i18n: wrote lang rc to %1").arg(user));
        return true;
    }
    appendLog(QString("i18n: failed to write lang rc to %1").arg(user));
    return false;
}

// ---------------------- end i18n helpers ----------------------

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

// Global event filter to force-close modal dialogs on window manager close
class ForceCloseFilter : public QObject
{
public:
    explicit ForceCloseFilter(QMainWindow* mainWin, QObject* parent = nullptr) 
        : QObject(parent), mainWindow(mainWin) {}
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        // If main window receives a close event, force close all dialogs immediately
        if (obj == mainWindow && event->type() == QEvent::Close) {
            // Find and force-close all dialogs
            QList<QDialog*> allDialogs = qApp->findChildren<QDialog*>();
            for (QDialog* dialog : allDialogs) {
                if (dialog->isVisible()) {
                    dialog->done(QDialog::Rejected);
                    dialog->hide();
                }
            }
            
            // Also close top-level widget dialogs
            QWidgetList topLevel = QApplication::topLevelWidgets();
            for (QWidget* widget : topLevel) {
                if (widget != mainWindow && widget->isVisible()) {
                    QDialog* dlg = qobject_cast<QDialog*>(widget);
                    if (dlg) {
                        dlg->done(QDialog::Rejected);
                        dlg->hide();
                    }
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
    
private:
    QMainWindow* mainWindow;
};

// Subclass QMainWindow to intercept closeEvent and show a cleaning dialog
class CleaningMainWindow : public QMainWindow
{
public:
    using QMainWindow::QMainWindow;

protected:
    bool event(QEvent *e) override
    {
        // Catch close event at the earliest stage - before it can be blocked by modal dialogs
        if (e->type() == QEvent::Close) {
            // Kill all dialogs immediately
            for (QWidget* w : QApplication::allWidgets()) {
                QDialog* dlg = qobject_cast<QDialog*>(w);
                if (dlg && dlg->isVisible()) {
                    dlg->done(QDialog::Rejected);
                }
            }
            
            // Do cleanup and force exit
            performCleanup();
            QCoreApplication::exit(0);
            e->accept();
            return true;
        }
        return QMainWindow::event(e);
    }
    
    void closeEvent(QCloseEvent *event) override
    {
        // Use deferred closing to break out of modal dialog event loops
        // Schedule immediate closing via timer to break modal blocking
        QTimer::singleShot(0, this, [this]() {
            // Close all top-level widgets (progress dialogs, etc.)
            QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
            for (QWidget* widget : topLevelWidgets) {
                if (widget != this && widget->isVisible()) {
                    QDialog* dialog = qobject_cast<QDialog*>(widget);
                    if (dialog) {
                        dialog->done(QDialog::Rejected);
                    }
                    widget->close();
                }
            }
            
            // Force close all child dialogs
            QList<QDialog*> dialogs = findChildren<QDialog*>();
            for (QDialog* dialog : dialogs) {
                dialog->done(QDialog::Rejected);
                dialog->close();
            }
            
            // Do quick cleanup and quit
            performCleanup();
            QApplication::quit();
        });
        
        // Accept immediately to allow the timer to fire
        event->accept();
        
        // Start cleanup in background. Only show the modal dialog if cleanup
        // takes longer than a short threshold (500 ms) to avoid a UI blink
        // on fast systems; on slower machines the dialog will appear and
        // remain until cleanup completes.
        QFuture<void> fut = QtConcurrent::run(performCleanup);
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
        watcher->setFuture(fut);

    // Prepare dialog but do not show it immediately.
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(QObject::tr("Cleaning up"));
    QVBoxLayout *lay = new QVBoxLayout(dlg);
    QLabel *lbl = new QLabel(QObject::tr("Cleaning up temporary files..."), dlg);
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
                QString label = translateTabName(config.name);
                m_tabWidget->addTab(tabWidget, label, config.name);
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
            // AudioTab doesn't use TabWidgetBase, so no loading signals
            tabWidget = audioTab;
        }
        else if (config.name == "Desktop") {
            WindowingTab* windowingTab = new WindowingTab();
            // WindowingTab doesn't use TabWidgetBase, so no loading signals
            tabWidget = windowingTab;
        }
        else if (config.name == "Graphics gard") {
            GraphicsTab* graphicsTab = new GraphicsTab();
            tabWidget = graphicsTab;
        }
        else if (config.name == "Screen") {
            ScreenTab* screenTab = new ScreenTab();
            tabWidget = screenTab;
        }
        else if (config.name == "Ports") {
            PortsTab* portsTab = new PortsTab();
            tabWidget = portsTab;
        }
        else if (config.name == "Peripherals") {
            PeripheralsTab* peripheralsTab = new PeripheralsTab();
            tabWidget = peripheralsTab;
        }
        else if (config.name == "Motherboard") {
            MotherboardTab* motherboardTab = new MotherboardTab();
            tabWidget = motherboardTab;
        }
        else if (config.name == "Disk") {
            appendLog("TabManager: Instantiating StorageTab");
            StorageTab* storageTab = new StorageTab();
            connect(storageTab, &TabWidgetBase::loadingStarted, this, &TabManager::onTabLoadingStarted);
            connect(storageTab, &TabWidgetBase::loadingFinished, this, &TabManager::onTabLoadingFinished);
            tabWidget = storageTab;
        }
        else if (config.name == "PC Info") {
            PCTab* pcTab = new PCTab();
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
            GenericTab* genericTab = new GenericTab(translateTabName(config.name), config.command, true, config.command);
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
    
    // Force Qt to use a light style on Linux Mint which has dark theme by default
    app.setStyle("Fusion");  // Use Fusion style instead of system default
    
    // Set light palette specifically for Mint's dark theme override
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::Text, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    app.setPalette(lightPalette);
    
    // Central version constant
    #include "version.h"

    // Load translator and determine language preference. Per user request
    // the application will ONLY consult a file named `lsv_lang.rc` that
    // lives in the same directory as the application binary. No other
    // paths (user config, parent dirs, etc.) are consulted.
    QTranslator translator;
    QSettings settings("LinuxSystemViewer", "LSV");
    QString savedLang;

    // Read primary RC (next to the application binary). If present it is
    // the authoritative language choice for this run/location. Do not
    // fall back to any other location.
    QString rcPrimary = readLangRcPrimary();
    if (!rcPrimary.isEmpty()) {
        savedLang = rcPrimary;
    } else {
        // If no primary RC, check user settings
        savedLang = settings.value("language", QString()).toString();
    }

    // Discover shipped languages and build a CLI-friendly list
    QStringList shipped = discoverShippedLanguageCodes();

    // Simple CLI parsing for language-related args
    QString requestedLang;
    bool chooseLang = false;
    bool resetLang = false;
    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == "--list-langs") {
            QStringList lines;
            QMap<QString, QString> names = shippedLanguageDisplayNames();
            // For publishing we only advertise English (UK) and Norwegian
            // Bokmål as the shipped languages.
            QStringList visible = QStringList() << "en_GB" << "nb";
            for (const QString &c : visible) {
                QString display = names.value(c, c);
                lines << QString("%1\t%2").arg(c, display);
            }
            QTextStream out(stdout);
            out << lines.join('\n') << '\n';
            return 0;
        } else if (a.startsWith("--lang=", Qt::CaseInsensitive)) {
            requestedLang = a.section('=', 1);
        } else if (a == "--choose-lang") {
            chooseLang = true;
        } else if (a == "--reset-lang") {
            resetLang = true;
        } else if (a == "--NO-nb") {
            requestedLang = "en"; // explicit negative flag to avoid nb
        }
    }

    // If chooseLang requested, show a simple choice dialog after elevation
    // (we set a flag here; the dialog will be shown later after elevation)
    bool showChooseDialogLater = chooseLang;

    // Determine effective language code to request.
    QString effectiveLang = requestedLang.isEmpty() ? savedLang : requestedLang;
    if (effectiveLang.isEmpty()) effectiveLang = "en"; // default

    // Try to load translator now (normalize codes like "is_IS" -> "is").
    if (!effectiveLang.isEmpty() && effectiveLang != "en") {
        QString actual = normalizeLanguageCodeToAvailable(effectiveLang);
        if (!actual.isEmpty()) {
            if (tryLoadTranslatorForCode(actual, app, &translator)) {
                // Record the actual code used in settings so the UI indicator
                // and future runs reflect the translator that was loaded.
                settings.setValue("language", actual);
            }
        } else {
            appendLog(QString("i18n: No available translator for requested language '%1', trying system locale").arg(effectiveLang));
            // Fallback to system locale if the saved language is invalid
            QString sysLocale = QLocale::system().name();
            QString sysActual = normalizeLanguageCodeToAvailable(sysLocale);
            if (!sysActual.isEmpty()) {
                if (tryLoadTranslatorForCode(sysActual, app, &translator)) {
                    appendLog(QString("i18n: Loaded system locale translator '%1'").arg(sysActual));
                    settings.setValue("language", sysActual);
                }
            }
        }
    }

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

    // Quick diagnostic flag: print the primary RC path (where the app
    // WILL look for lsv_lang.rc for this invocation) and exit. This is
    // intentionally handled before auto-elevation so you can inspect the
    // path without triggering sudo relaunches.
    QStringList earlyArgs = QCoreApplication::arguments();
    if (earlyArgs.contains("--rc-path")) {
        QString p = langRcPrimaryPath();
        QTextStream out(stdout);
        out << p << '\n';
        // Also indicate whether the file exists and print its contents for convenience
        if (!p.isEmpty() && QFile::exists(p)) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                out << "EXISTS\n";
                out << f.readAll() << '\n';
                f.close();
            }
        } else {
            out << "MISSING\n";
        }
        return 0;
    }

    // Handle reset-lang CLI flag: remove the per-user language RC and exit
    if (resetLang) {
        QString p = langRcFilePath();
        QTextStream out(stdout);
        if (p.isEmpty()) {
            out << "ERROR: no config path\n";
            return 1;
        }
        QFile f(p);
        if (!f.exists()) {
            out << "MISSING\n";
            return 0;
        }
        if (QFile::remove(p)) {
            out << "OK\n";
            return 0;
        } else {
            out << "FAILED\n";
            return 1;
        }
    }

    // Auto-elevation: always relaunch via a terminal sudo prompt and exit the
    // unprivileged instance. This ensures the user always authenticates in a
    // terminal window with a clear custom message and the GUI runs as root.
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
    // If running from a transient mount, try to pre-copy the binary so
    // root can execute it even if the transient mount becomes inaccessible.
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

        // Prefer using pkexec (Polkit GUI) first so users get a native
        // privilege prompt. If pkexec is not available or fails, fall back
        // to the terminal-based sudo wrapper used previously.
        // Note: pkexec may strip environment; we pass minimal display
        // environment variables explicitly.
        // For reliable behavior when launched from the desktop/file-manager
        // we prefer the terminal-based elevation flow. Historically we used
        // pkexec (Polkit) which can fail silently if no GUI authentication
        // agent is running in the user's session. To avoid the "Request
        // dismissed" cases where the user can't start the app from the menu
        // we always use the terminal sudo wrapper here.
        appendLog("Auto-elevation: pkexec disabled by policy; using terminal sudo fallback for menu launches.");

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
            QMessageBox::critical(nullptr, QObject::tr("Cannot elevate"), QObject::tr("No terminal emulator found to prompt for a password.\nPlease run the application as root."));
            return 0;
        }

        // Build the sudo command that authenticates and then starts the GUI
        // as a detached process so the terminal can close after auth.
        QString sudoPrompt = QCoreApplication::translate("QObject", "Please enter password to run Linux System Viewer as root");
        QString escTarget = targetExe;
        escTarget.replace('\'', "'" "'" "'");
        QString inner = QString("setsid '%1' > /dev/null 2>&1 &").arg(escTarget);
        // Create a temporary wrapper script to run sudo. This reduces quoting
        // issues when passing complex commands to terminal emulators.
        QString wrapperPath = QDir::tempPath() + QDir::separator() + QString("lsv-sudo-%1.sh").arg(getpid());
        // preferredCols is used later when deciding terminal geometry;
        // declare it here so it stays in scope outside the writer block.
        int preferredCols = 80;
        QFile wf(wrapperPath);
            if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream ts(&wf);
            ts << "#!/bin/bash\n";
            // Ensure the wrapper removes itself and its temporary log on exit
            ts << "trap \"rm -f '" << wrapperPath.replace('\'', "'\"'\"'") << "' /tmp/lsv-relaunch-" << getpid() << ".log\" EXIT\n";
            ts << "echo 'LSV wrapper starting at ' $(date) > /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "echo 'Running sudo to start LSV as root' >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            // Prompt and allow up to 3 attempts. Use read -s so Enter works
            // normally and let sudo validate. On success exit; after 3 bad
            // attempts give up.
            // Export important X/Wayland/display env so the elevated process
            // can connect to the user's display. We capture current values
            // and write them into the wrapper script.
            QString envDisplay = QString::fromLocal8Bit(qgetenv("DISPLAY"));
            QString envXAuth = QString::fromLocal8Bit(qgetenv("XAUTHORITY"));
            QString envXdg = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
            if (!envDisplay.isEmpty()) ts << "export DISPLAY='" << envDisplay.replace('\'', "'\"'\"'") << "'\n";
            if (!envXAuth.isEmpty()) ts << "export XAUTHORITY='" << envXAuth.replace('\'', "'\"'\"'") << "'\n";
            if (!envXdg.isEmpty()) ts << "export XDG_RUNTIME_DIR='" << envXdg.replace('\'', "'\"'\"'") << "'\n";
            
            // Export the original user's home directory so the elevated instance
            // can access the same language RC file as the non-elevated instance.
            QString userHome = QDir::homePath();
            QString userConfig = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
            if (!userHome.isEmpty()) ts << "export LSV_USER_HOME='" << userHome.replace('\'', "'\"'\"'") << "'\n";
            if (!userConfig.isEmpty()) ts << "export LSV_USER_CONFIG='" << userConfig.replace('\'', "'\"'\"'") << "'\n";

            // Export the original application directory so an elevated
            // instance can still locate the original binary directory and
            // any portable RC file next to it. Prefer an existing
            // LSV_ORIG_APPDIR value (if present), otherwise export the
            // directory containing the current executable.
            QString origEnv = QString::fromLocal8Bit(qgetenv("LSV_ORIG_APPDIR"));
            QString origAppDir;
            if (!origEnv.isEmpty()) origAppDir = origEnv;
            else origAppDir = QFileInfo(exe).absolutePath();
            if (!origAppDir.isEmpty()) ts << "export LSV_ORIG_APPDIR='" << origAppDir.replace('\'', "'\"'\"'\"") << "'\n";

            QString promptEsc = sudoPrompt;
            // Escape any double-quotes so we can emit the prompt inside a
            // double-quoted printf without breaking the wrapper script.
            promptEsc.replace('"', "\\\"");
            // Compute preferred terminal width (columns) based on prompt
            // length so the window is just wide enough. Keep a sensible
            // minimum to avoid cramped terminals and a maximum for very
            // long prompts.
            preferredCols = promptEsc.size() + 6; // padding
            if (preferredCols < 40) preferredCols = 40;
            if (preferredCols > 100) preferredCols = 100;
            int pad = (preferredCols - (int)promptEsc.size()) / 2;
            if (pad < 0) pad = 0;
            int padRight = preferredCols - pad - (int)promptEsc.size();
            if (padRight < 0) padRight = 0;
            QString padStr(pad, ' ');
            QString padRightStr(padRight, ' ');
            // Try to set terminal colors where supported. Note: not all
            // terminals apply these to the titlebar; OSC 10/11 set text/bg.
            ts << "printf '\033]10;#000000\\007'\n"; // foreground black
            ts << "printf '\033]11;#F3F3F4\\007'\n"; // background light gray
            // Set the terminal window title to include the application
            // version so users can confirm which release they're elevating.
            QString termTitle = QCoreApplication::translate("QObject", "Linux System Viewer %1").arg(LSVVersionQString());
            QString termTitleEsc = termTitle;
            termTitleEsc.replace('\'', "'\"'\"'");
            ts << "printf '\\033]0;" << termTitleEsc << "\\007'\n";
            ts << "attempts=0\n";
            ts << "while [ $attempts -lt 3 ]; do\n";
            ts << "  attempts=$((attempts+1))\n";
            // Print centered prompt on the first line. Leave the second
            // line with the same left padding and immediately read the
            // password there (so the typed characters are aligned under
            // the prompt). Leave the third line blank for error messages.
            // Line 1: left padding + prompt + right padding (equal space both sides)
            // Print the full centered prompt line in a single printf so
            // shells receive one well-formed command. Use ANSI SGR to set
            // the prompt text color to #001675 and reset afterwards.
            ts << "printf '%s\\033[38;2;0;22;117m%s\\033[0m%s\\n' \"" << padStr << "\" \"" << promptEsc << "\" \"" << padRightStr << "\"\n";
            // Line 2: print left padding, a centered marker (colored #B10000),
            // then right padding WITHOUT emitting a newline so we can
            // reposition the cursor back to the marker column and read
            // input there. The read is silent so the marker remains visible
            // while the user types. This preserves the third line for error
            // messages.
            // Compute marker padding so the marker is exactly centered in
            // the terminal frame regardless of the prompt width.
            int markerPadLeft = (preferredCols - 1) / 2;
            if (markerPadLeft < 0) markerPadLeft = 0;
            int markerPadRight = preferredCols - markerPadLeft - 1;
            if (markerPadRight < 0) markerPadRight = 0;
            QString markerLeft(markerPadLeft, ' ');
            QString markerRight(markerPadRight, ' ');
            ts << "printf '%s\\033[38;2;177;0;0m%s\\033[0m%s' \"" << markerLeft << "\" \"" << ">" << "\" \"" << markerRight << "\"\n";
            // Move cursor to start of line then print left padding so the
            // user's input begins centered under the marker. Use the
            // marker's left padding (markerLeft) so the cursor aligns with
            // the visual center instead of the prompt's left padding.
            ts << "printf '\r'\n";
            ts << "printf '%s' \"" << markerLeft << "\"\n";
            ts << "read -s PASS\n";
            // Line 3: empty line reserved for error messages
            ts << "echo\n";
            QString innerEsc = inner;
            innerEsc.replace('"', "\\\"");
            // Pass through important environment variables via sudo.
            // sudo resets the environment, so we must explicitly preserve
            // DISPLAY, XAUTHORITY, XDG_RUNTIME_DIR, LSV_USER_CONFIG, etc.
            ts << "  printf '%s\\n' \"$PASS\" | sudo -S -p '' \\\n";
            ts << "    DISPLAY=\"$DISPLAY\" \\\n";
            ts << "    XAUTHORITY=\"$XAUTHORITY\" \\\n";
            ts << "    XDG_RUNTIME_DIR=\"$XDG_RUNTIME_DIR\" \\\n";
            ts << "    LSV_USER_HOME=\"$LSV_USER_HOME\" \\\n";
            ts << "    LSV_USER_CONFIG=\"$LSV_USER_CONFIG\" \\\n";
            ts << "    LSV_ORIG_APPDIR=\"$LSV_ORIG_APPDIR\" \\\n";
            ts << "    LSV_ELEVATED=1 \\\n";
            ts << "    sh -c \"" << innerEsc << "\"\n";
            ts << "  rc=$?\n";
            ts << "  echo 'sudo finished with exitcode:' $rc >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "  if [ $rc -eq 0 ]; then exit 0; fi\n";
            QString authFailedMsg = QCoreApplication::translate("QObject", "Authentication failed (%1/3)");
            authFailedMsg.replace('"', "\\\"");
            ts << "  printf '" << authFailedMsg.replace("%1", "'$attempts'") << "\\n' >&2\n";
            ts << "done\n";
            ts << "echo 'Giving up after 3 failed attempts' >> /tmp/lsv-relaunch-" << getpid() << ".log\n";
            ts << "exit 1\n";
            wf.close();
            QFile::setPermissions(wrapperPath, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
        } else {
            appendLog(QString("Auto-elevation: Failed to write wrapper script %1").arg(wrapperPath));
        }

    QStringList args;
    // Use a geometry matching the preferred columns computed earlier
    // so the terminal width matches the prompt. Height remains 3 rows
    // (prompt, input, and spare line for errors).
    QString base = QFileInfo(termPath).fileName();
    QString geom = QString("%1x3").arg(preferredCols); // width x height
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
            QMessageBox::critical(nullptr, QObject::tr("Elevation failed"), QObject::tr("Failed to start a terminal to request sudo password. Please run the application as root."));
        }

        // Exit the unprivileged instance immediately; the elevated GUI will
        // be started from the terminal after authentication.
        return 0;
    }

    // If requested, show a language chooser dialog after elevation and before
    // creating the main window. When running elevated (geteuid()==0) the GUI
    // is about to be shown — this is the right place to show the chooser so
    // the user can pick the desired language before the main UI appears.
    // If running elevated (GUI about to be shown) and the persistent
    // language rc file doesn't exist, or the user explicitly requested
    // a choice, show the chooser. When the user confirms we write the
    // chosen code into the rc file so it becomes the default for future
    // runs. To see the chooser again, delete the rc file.
    // Show chooser when elevated if the primary RC file next to the
    // application binary is missing (or when explicitly requested). We
    // check the primary path so the chooser reappears when `lsv_lang.rc`
    // is not present next to the binary, even if a fallback user config
    // file exists.
    QString primaryRc = langRcPrimaryPath();
    appendLog(QString("i18n: LSV_ORIG_APPDIR='%1', applicationDirPath='%2', primaryRc='%3'")
              .arg(QString::fromLocal8Bit(qgetenv("LSV_ORIG_APPDIR")), QCoreApplication::applicationDirPath(), primaryRc));
    appendLog(QString("i18n: primaryRc exists=%1 chooseLang=%2 geteuid=%3").arg(QString::number(QFile::exists(primaryRc)), chooseLang ? "true" : "false", QString::number(geteuid())));

    if (geteuid() == 0 && (chooseLang || !QFile::exists(primaryRc))) {
        QMap<QString, QString> names = shippedLanguageDisplayNames();
        QStringList choices;
        QStringList codes = discoverShippedLanguageCodes();
        // Ensure English is the default selection in the dialog
        codes.removeAll("en_GB");
        codes.removeAll("en");
        codes.prepend("en_GB");
        for (const QString &c : codes) choices << names.value(c, c);
        // Determine the default index (English preferred)
        int defaultIndex = 0;
        for (int i = 0; i < codes.size(); ++i) {
            if (codes.at(i) == "en" || codes.at(i) == "en_GB") { defaultIndex = i; break; }
        }
        bool ok = false;
        QString pick = QInputDialog::getItem(nullptr, QObject::tr("Choose language"), QObject::tr("Language:"), choices, defaultIndex, false, &ok);
        if (ok && !pick.isEmpty()) {
            int idx = choices.indexOf(pick);
            QString code = (idx >= 0 && idx < codes.size()) ? codes.at(idx) : pick;
            // Persist the selection to the rc file so it becomes the default
            // for future runs. Also store in QSettings for internal consistency.
            // Normalize the requested code to an available translator
            // (e.g. is_IS -> is) before persisting and loading.
            QString actual = normalizeLanguageCodeToAvailable(code);
            QString toWrite = actual.isEmpty() ? QString("en") : actual;
            if (!writeLangRc(toWrite)) {
                QMessageBox::warning(nullptr, QObject::tr("Language selection"), QObject::tr("Failed to write language selection to %1").arg(primaryRc));
            }
            settings.setValue("language", toWrite);
            // Reload translator if needed
            app.removeTranslator(&translator);
            if (toWrite != "en") {
                tryLoadTranslatorForCode(toWrite, app, &translator);
            }
        }
    }

    // Create main window
    CleaningMainWindow mainWindow;
    mainWindow.setWindowTitle(QObject::tr("Linux System Viewer V. %1").arg(LSVVersionQString()));
    mainWindow.setWindowIcon(appIcon);
    
    // Install event filter to force-close dialogs on window manager close
    ForceCloseFilter* closeFilter = new ForceCloseFilter(&mainWindow, &mainWindow);
    mainWindow.installEventFilter(closeFilter);

    // Set initial size (850x516) but adapt to available screen space
    // Increased height to accommodate Summary tab without scrolling.
    const int INITIAL_WIDTH = 850;
    const int INITIAL_HEIGHT = 516;

    // Prefer an adaptive height: use the smaller of INITIAL_HEIGHT and
    // (available screen height - margin). This avoids the window being
    // placed partially under desktop panels on some setups (Cinnamon/GNOME).
    int chosenHeight = INITIAL_HEIGHT;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        int avail = screen->availableGeometry().height();
        // Leave a small margin for panels (48px) and ensure a sensible minimum
        const int PANEL_MARGIN = 48;
        const int MIN_HEIGHT = 360;
        if (avail > PANEL_MARGIN + MIN_HEIGHT) {
            chosenHeight = qMin(INITIAL_HEIGHT, avail - PANEL_MARGIN);
            chosenHeight = qMax(MIN_HEIGHT, chosenHeight);
        }
    }

    mainWindow.resize(INITIAL_WIDTH, chosenHeight);
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
    // Headline: show product name only (avoid repeating the version here).
    // Show product name in the title row (version shown in elevation terminal)
    QLabel* titleLabel = new QLabel(QObject::tr("Linux System Viewer"));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    // Increase the main headline by ~30% for better visual prominence.
    // Use the current font point size when available, otherwise fall back to 12.
    qreal basePointSize = titleFont.pointSizeF();
    if (basePointSize <= 0) basePointSize = 12.0;
    titleFont.setPointSizeF(basePointSize * 1.3);
    titleLabel->setFont(titleFont);
    // Center the headline text horizontally in the title area
    titleLabel->setAlignment(Qt::AlignCenter);

    // About button (colored badge): blue for normal user, green for superuser
    QToolButton* aboutBtn = new QToolButton;
    QColor bg = (geteuid() == 0) ? QColor("#2ecc71") : QColor("#3498db");
    QPixmap badge = makeBadgePixmap(bg, 20);
    aboutBtn->setIcon(QIcon(badge));
    aboutBtn->setIconSize(QSize(20,20));
    aboutBtn->setAutoRaise(true);
    aboutBtn->setToolTip(QObject::tr("About Linux System Viewer"));
    QObject::connect(aboutBtn, &QAbstractButton::clicked, [&app]() {
        // Show AboutTab as a top-level window/dialog
        AboutTab* about = new AboutTab();
        about->setAttribute(Qt::WA_DeleteOnClose);
        about->setWindowModality(Qt::ApplicationModal);
        about->show();
        about->raise();
        about->activateWindow();
    });

    // NOTE: language chooser dropdown removed from the title bar in favor
    // of the Administration menu actions (menu is translated via QObject::tr).

    // Helper: apply a language code, reload translator and recreate tabs so
    // the whole UI updates immediately. preserveIndex indicates which tab
    // index should be selected after recreation (use -1 to ignore).
    auto applyLanguage = [&mainWindow, &app, &translator, &settings, &titleLabel, &aboutBtn](const QString &code, int preserveIndex = -1) {
        // Remove any installed translator then load the requested one. Use
        // a normalized code so region variants fall back to available
        // translator files (is_IS -> is).
        QString actual = normalizeLanguageCodeToAvailable(code);
        app.removeTranslator(&translator);
        if (!actual.isEmpty()) {
            if (tryLoadTranslatorForCode(actual, app, &translator)) {
                settings.setValue("language", actual);
            }
        } else {
            // No translator: use English and record that choice
            settings.setValue("language", "en");
        }

        // Update small UI bits
        mainWindow.setWindowTitle(QObject::tr("Linux System Viewer V. %1").arg(LSVVersionQString()));
        if (titleLabel) titleLabel->setText(QObject::tr("Linux System Viewer"));
        if (aboutBtn) aboutBtn->setToolTip(QObject::tr("About Linux System Viewer"));

        // Retranslate Language menu and actions so menu text updates
        // immediately without restarting the application.
        QMenuBar* mb = mainWindow.menuBar();
        if (mb) {
            QMenu* adminMenu = mb->findChild<QMenu*>("adminMenu");
            if (adminMenu) adminMenu->setTitle(QObject::tr("Language"));
            QAction* changeAct = mb->findChild<QAction*>("changeLangAct");
            if (changeAct) changeAct->setText(QObject::tr("Change language..."));
            QAction* resetAct = mb->findChild<QAction*>("resetLangAct");
            if (resetAct) resetAct->setText(QObject::tr("Reset language"));
            // Update the language indicator badge text as well
            QLabel* indicator = mb->findChild<QLabel*>("langIndicator");
                if (indicator) {
                    QMap<QString, QString> names2 = shippedLanguageDisplayNames();
                    QString currentLang = settings.value("language", QString("en")).toString();
                    QString display = names2.value(currentLang, currentLang);
                    indicator->setText(display);
                }
        }

        // Recreate the tab widget to ensure constructor-time tr() calls run
        // with the newly installed translator. Preserve the currently
        // selected tab index where possible.
        MultiRowTabWidget* oldTab = mainWindow.findChild<MultiRowTabWidget*>();
        QWidget* central = mainWindow.centralWidget();
        QLayout* ml = central ? central->layout() : nullptr;
        if (oldTab && ml) {
            int curIndex = (preserveIndex >= 0) ? preserveIndex : oldTab->currentIndex();
            oldTab->shutdownTabs();
            ml->removeWidget(oldTab);
            oldTab->setParent(nullptr);
            oldTab->deleteLater();

            // Remove any existing TabManager children (they are parented
            // to mainWindow by construction earlier).
            TabManager* oldMgr = mainWindow.findChild<TabManager*>();
            if (oldMgr) oldMgr->deleteLater();

            MultiRowTabWidget* newTab = new MultiRowTabWidget();
            ml->addWidget(newTab);
            TabManager* newMgr = new TabManager(&mainWindow);
            newMgr->setTabWidget(newTab);
            QObject::connect(newMgr, &TabManager::tabLoadingStarted, [](const QString& tabName){ qDebug() << "Loading started for tab:" << tabName; });
            QObject::connect(newMgr, &TabManager::tabLoadingFinished, [](const QString& tabName){ qDebug() << "Loading finished for tab:" << tabName; });
            QTimer::singleShot(0, [newMgr, curIndex]() {
                newMgr->createAllTabs();
                MultiRowTabWidget* nt = newMgr->parent()->findChild<MultiRowTabWidget*>();
                if (nt && curIndex >= 0) nt->setCurrentIndex(curIndex);
            });
        }
    };

    // Language changes are handled via the Language menu actions.

    // Reset handled via Language menu action; no title-button required.

    // Place stretches on both sides of the title so it stays centered
    // while keeping the About button anchored to the right edge.
    titleLayout->addStretch();
    titleLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    titleLayout->addStretch();
    titleLayout->addWidget(aboutBtn);
    mainLayout->addLayout(titleLayout);

    // Add a "Language" menu with language actions so the language
    // chooser is reachable from the menu as well (useful on translated
    // desktops where the title-area combo may be less discoverable).
    QMenuBar* mb = mainWindow.menuBar();
    QMenu* adminMenu = mb->addMenu(QObject::tr("Language"));
    adminMenu->setObjectName("adminMenu");
    QAction* changeLangAct = adminMenu->addAction(QObject::tr("Change language..."));
    changeLangAct->setObjectName("changeLangAct");
    QAction* resetLangAct = adminMenu->addAction(QObject::tr("Reset language"));
    resetLangAct->setObjectName("resetLangAct");

    // Language indicator shown in the menu bar: display current language
    // in a small blue badge (native name). This is a non-interactive
    // widget added as a QWidgetAction so it appears alongside the menus.
    QMap<QString, QString> names = shippedLanguageDisplayNames();
    QString curLang = settings.value("language", QString()).toString();
    if (curLang.isEmpty()) curLang = "en";
    QWidgetAction* langIndicatorAct = new QWidgetAction(mb);
    QLabel* langIndicator = new QLabel(names.value(curLang, curLang));
    langIndicator->setObjectName("langIndicator");
    // Use the pleasant blue consistent with other UI accents
    langIndicator->setStyleSheet("QLabel#langIndicator { background-color: #1E88E5; color: white; padding: 3px 8px; border-radius: 6px; font-weight: bold; }");
    langIndicatorAct->setDefaultWidget(langIndicator);
    mb->addAction(langIndicatorAct);

    QObject::connect(changeLangAct, &QAction::triggered, [&mainWindow, &settings, &applyLanguage]() {
        QMap<QString, QString> names = shippedLanguageDisplayNames();
        // All supported languages sorted alphabetically by display name:
        // Dansk, Deutsch, Ελληνικά, English (UK), Español, Français, Íslenska, Norsk (Bokmål), Suomi, Svenska
        QStringList codes;
        codes << "da" << "de" << "el" << "en_GB" << "es" << "fi" << "fr" << "is" << "nb" << "sv";
        QStringList choices;
        for (const QString &c : codes) choices << names.value(c, c);

        // Preselect the currently active language where possible and show
        // it prominently in the dialog label even when the active language
        // is not part of the published choices (for example when the
        // per-user RC contains 'is').
        QString curLang = settings.value("language", QString()).toString();
        if (curLang.isEmpty()) curLang = "en_GB";
        int defaultIndex = codes.indexOf(curLang);
        if (defaultIndex < 0) defaultIndex = codes.indexOf("en_GB");

        bool ok = false;
        QString labelText = QObject::tr("Language:");
        QString pick = QInputDialog::getItem(nullptr, QObject::tr("Choose language"), labelText, choices, defaultIndex, false, &ok);
        if (!ok || pick.isEmpty()) return;
        int idx = choices.indexOf(pick);
        QString code = (idx >= 0 && idx < codes.size()) ? codes.at(idx) : pick;
        // Normalize to an available translator before persisting/loading
        QString actual = normalizeLanguageCodeToAvailable(code);
        QString toWrite = actual.isEmpty() ? QString("en") : actual;
        if (!writeLangRc(toWrite)) {
            QMessageBox::warning(nullptr, QObject::tr("Language selection"), QObject::tr("Failed to write language selection to configuration directory"));
        }
        settings.setValue("language", toWrite);

        // Apply the language change and preserve current tab selection.
        MultiRowTabWidget* oldTab = mainWindow.findChild<MultiRowTabWidget*>();
        int curIndex = oldTab ? oldTab->currentIndex() : -1;
        applyLanguage(toWrite, curIndex);

        Q_UNUSED(code);
        QMessageBox::information(nullptr, QObject::tr("Language changed"), QObject::tr("Language saved. UI updated to the selected language."));
    });

    QObject::connect(resetLangAct, &QAction::triggered, [&settings, &applyLanguage, &mainWindow]() {
        QString rc = langRcFilePath();
        if (!rc.isEmpty() && QFile::exists(rc)) {
            if (!QFile::remove(rc)) {
                QMessageBox::warning(nullptr, QObject::tr("Reset language"), QObject::tr("Failed to remove %1").arg(rc));
                return;
            }
        }
        settings.setValue("language", "en");

        MultiRowTabWidget* oldTab = mainWindow.findChild<MultiRowTabWidget*>();
        int curIndex = oldTab ? oldTab->currentIndex() : -1;
        applyLanguage("en", curIndex);

        Q_UNUSED(settings);
        QMessageBox::information(nullptr, QObject::tr("Reset language"), QObject::tr("Saved language selection removed. The application is now using English."));
    });

    // Create tab widget
    MultiRowTabWidget* tabWidget = new MultiRowTabWidget();
    mainLayout->addWidget(tabWidget);

    // Install global Ctrl+W / close handler so Ctrl+W shows a quit dialog
    // and window-close events can be intercepted. The handler is parented
    // to the main window so it lives as long as the window does.
    CtrlWHandler* ctrlHandler = new CtrlWHandler(&mainWindow, &mainWindow);
    Q_UNUSED(ctrlHandler);

    // Create initial TabManager on the heap and parent it to mainWindow so
    // it can be safely deleted and recreated when the UI language changes.
    TabManager* tabManager = new TabManager(&mainWindow);
    tabManager->setTabWidget(tabWidget);

    // Status connections (use the heap-managed tabManager)
    QObject::connect(tabManager, &TabManager::tabLoadingStarted, [](const QString& tabName) {
        qDebug() << "Loading started for tab:" << tabName;
    });

    QObject::connect(tabManager, &TabManager::tabLoadingFinished, [](const QString& tabName) {
        qDebug() << "Loading finished for tab:" << tabName;
    });

    // Show main window first so the UI appears even if tab construction takes time.
    mainWindow.show();
    qDebug() << "Application window shown, scheduling tab creation...";

    // Defer heavy tab creation to the event loop so the window can render immediately.
    QTimer::singleShot(0, [tabManager]() {
        qDebug() << "Creating tabs...";
        tabManager->createAllTabs();
        qDebug() << "All tabs created successfully";
    });

    qDebug() << "Entering event loop...";
    return app.exec();
}

#include "lsv.moc"
#include <QtWidgets>
#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QTranslator>
#include <QSettings>
#include <QLibraryInfo>
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
#include "elevation_dialog.h"
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
    // Ship English (UK), German, Spanish, French, Norwegian Bokmål, Icelandic, Italian, Greek, Danish, Finnish, Swedish, and Basque in the UI list.
    // The repository and packaging contain these translators
    // and the application presents these languages to users.
    m.insert("en_GB", "English (UK)");
    m.insert("en", "English (UK)");
    m.insert("da", "Dansk");
    m.insert("de", "Deutsch");
    m.insert("el", "Ελληνικά");
    m.insert("es", "Español");
    m.insert("eu", "Euskara");
    m.insert("fi", "Suomi");
    m.insert("fr", "Français");
    m.insert("nb", "Norsk (Bokmål)");
    m.insert("is", "Íslenska");
    m.insert("it", "Italiano");
    m.insert("sv", "Svenska");
    m.insert("zh_CN", "简体中文");
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

// Load Qt's base translations for standard widgets (Cancel, OK, etc.)
static bool tryLoadQtBaseTranslator(const QString &code, QApplication &app, QTranslator *qtTranslator)
{
    if (code == "en") return false; // English = default, no translator needed
    
    // Qt translations are usually installed in QLibraryInfo::TranslationsPath
    QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    
    // Try loading qtbase translations for this language
    QStringList candidates;
    candidates << QString("%1/qtbase_%2.qm").arg(qtTranslationsPath, code);
    // Also try without country code (e.g., "nb" instead of "nb_NO")
    if (code.contains('_')) {
        QString shortCode = code.section('_', 0, 0);
        candidates << QString("%1/qtbase_%2.qm").arg(qtTranslationsPath, shortCode);
    }
    
    for (const QString &p : candidates) {
        if (QFile::exists(p) && qtTranslator->load(p)) {
            app.installTranslator(qtTranslator);
            appendLog(QString("i18n: Loaded Qt base translator for '%1' from %2").arg(code, p));
            return true;
        }
    }
    
    appendLog(QString("i18n: No Qt base translator found for '%1' (tried %2)").arg(code, qtTranslationsPath));
    return false;
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

// Unified language config: always use ~/.config/LinuxSystemViewer/LSV.conf and QSettings
static QString userConfigPath() {
    QString envUserConfig = QString::fromLocal8Bit(qgetenv("LSV_USER_CONFIG"));
    if (!envUserConfig.isEmpty()) {
        return envUserConfig;
    }
    return QDir::homePath() + "/.config/LinuxSystemViewer/LSV.conf";
}

static QString readLanguageFromConfig() {
    QSettings settings(userConfigPath(), QSettings::IniFormat);
    return settings.value("language", QString()).toString();
}

static void writeLanguageToConfig(const QString &lang) {
    QSettings settings(userConfigPath(), QSettings::IniFormat);
    settings.setValue("language", lang);
    settings.sync();
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
    QTranslator qtTranslator;  // For Qt's built-in widgets (Cancel, OK, etc.)
    // Always use explicit config path in user's home for settings, allow override from environment
    QString userConfig = QDir::homePath() + "/.config/LinuxSystemViewer/LSV.conf";
    QByteArray envConfig = qgetenv("LSV_USER_CONFIG");
    if (!envConfig.isEmpty()) {
        userConfig = QString::fromLocal8Bit(envConfig);
    }
    QSettings settings(userConfig, QSettings::IniFormat);
    QString savedLang = settings.value("language", QString()).toString();

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
                // Also load Qt's base translations for standard widgets
                tryLoadQtBaseTranslator(actual, app, &qtTranslator);
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
                    // Also load Qt's base translations
                    tryLoadQtBaseTranslator(sysActual, app, &qtTranslator);
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

    // Remove obsolete --rc-path and reset-lang CLI logic

    // Auto-elevation: if not running as root, show custom elevation dialog
    // This provides a consistent graphical authentication experience across
    // all Linux distributions without depending on pkexec or terminal emulators.
    if (geteuid() != 0 && qgetenv("LSV_ELEVATED").isEmpty()) {
        // Get current executable path
        QString exe = QCoreApplication::applicationFilePath();
        
        // Handle transient mounts (e.g., AppImage)
        QString targetExe = exe;
        if (exe.contains("/tmp/.mount_")) {
            QString preCopiedExe = QDir::tempPath() + QDir::separator() + 
                                   QString("lsv-elevated-%1").arg(QCoreApplication::applicationPid());
            if (QFile::copy(exe, preCopiedExe)) {
                QFile::setPermissions(preCopiedExe, 
                                     QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner |
                                     QFile::ExeGroup | QFile::ReadGroup |
                                     QFile::ExeOther | QFile::ReadOther);
                targetExe = preCopiedExe;
                appendLog(QString("Auto-elevation: Copied mounted exe to %1").arg(preCopiedExe));
            }
        }

        // Show custom elevation dialog
        appendLog("Auto-elevation: Showing custom elevation dialog");
        
        if (!ElevationDialog::elevateAndRestart(targetExe)) {
            // User cancelled or authentication failed
            appendLog("Auto-elevation: User cancelled or authentication failed");
            QMessageBox::information(nullptr, 
                                    QObject::tr("Authentication Required"),
                                    QObject::tr("Linux System Viewer requires root privileges to access hardware information.\n\n"
                                              "You can also run it from terminal with: sudo lsv"));
            return 0;
        }
        
        // If we get here, the elevated instance was started and we should exit
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
    // Show language selector ONLY if config file does not exist (not if language is missing)
    bool configExists = QFile::exists(userConfig);
    if (geteuid() == 0 && (chooseLang || !configExists)) {
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
        // Custom dialog with translated buttons (Cancel/OK)
        QDialog dialog(nullptr);
        dialog.setWindowTitle(QObject::tr("Choose language"));
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        QLabel* label = new QLabel(QObject::tr("Language:"));
        layout->addWidget(label);
        
        QComboBox* comboBox = new QComboBox();
        comboBox->addItems(choices);
        comboBox->setCurrentIndex(defaultIndex);
        layout->addWidget(comboBox);
        
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton(QObject::tr("OK"));
        QPushButton* cancelButton = new QPushButton(QObject::tr("Cancel"));
        buttonLayout->addStretch();
        buttonLayout->addWidget(cancelButton);
        buttonLayout->addWidget(okButton);
        layout->addLayout(buttonLayout);
        
        QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        bool ok = (dialog.exec() == QDialog::Accepted);
        QString pick = ok ? comboBox->currentText() : QString();
        if (ok && !pick.isEmpty()) {
            int idx = choices.indexOf(pick);
            QString code = (idx >= 0 && idx < codes.size()) ? codes.at(idx) : pick;
            // Persist the selection to QSettings only
            QString actual = normalizeLanguageCodeToAvailable(code);
            QString toWrite = actual.isEmpty() ? QString("en") : actual;
            settings.setValue("language", toWrite);
            settings.sync();
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

    // Language button (styled like Geek mode button) for language selection
    QPushButton* langBtn = new QPushButton(QObject::tr("Language"));
    langBtn->setObjectName("langBtn");
    langBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 11px; min-width: 80px; max-height: 22px;}"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    langBtn->setFixedHeight(22);

    // Helper: apply a language code, reload translator and recreate tabs so
    // the whole UI updates immediately. preserveIndex indicates which tab
    // index should be selected after recreation (use -1 to ignore).
    auto applyLanguage = [&mainWindow, &app, &translator, &qtTranslator, &settings, &titleLabel, &aboutBtn, &langBtn](const QString &code, int preserveIndex = -1) {
        // Remove any installed translator then load the requested one. Use
        // a normalized code so region variants fall back to available
        // translator files (is_IS -> is).
        QString actual = normalizeLanguageCodeToAvailable(code);
        app.removeTranslator(&translator);
        app.removeTranslator(&qtTranslator);
        if (!actual.isEmpty()) {
            if (tryLoadTranslatorForCode(actual, app, &translator)) {
                settings.setValue("language", actual);
                settings.sync();  // Force write to disk
                appendLog(QString("i18n: applyLanguage saved '%1' to QSettings: %2").arg(actual, settings.fileName()));
                // Also load Qt's base translations
                tryLoadQtBaseTranslator(actual, app, &qtTranslator);
            }
        } else {
            // No translator: use English and record that choice
            settings.setValue("language", "en");
            settings.sync();  // Force write to disk
            appendLog(QString("i18n: applyLanguage saved 'en' to QSettings: %1").arg(settings.fileName()));
        }

        // Update small UI bits
        mainWindow.setWindowTitle(QObject::tr("Linux System Viewer V. %1").arg(LSVVersionQString()));
        if (titleLabel) titleLabel->setText(QObject::tr("Linux System Viewer"));
        if (aboutBtn) aboutBtn->setToolTip(QObject::tr("About Linux System Viewer"));
        if (langBtn) langBtn->setText(QObject::tr("Language"));

        // No menu bar needed - language button handles selection
        {
                    QMap<QString, QString> names2 = shippedLanguageDisplayNames();
                    QString currentLang = settings.value("language", QString("en")).toString();
                    QString display = names2.value(currentLang, currentLang);
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

    // Connect Language button to show language chooser dialog
    QObject::connect(langBtn, &QPushButton::clicked, [&mainWindow, &settings, &applyLanguage]() {
        QMap<QString, QString> names = shippedLanguageDisplayNames();
        // All supported languages sorted alphabetically by display name:
        // Dansk, Deutsch, Ελληνικά, English (UK), Español, Euskara, Français, Íslenska, Italiano, Norsk (Bokmål), Suomi, Svenska, 简体中文
        QStringList codes;
        codes << "da" << "de" << "el" << "en_GB" << "es" << "eu" << "fi" << "fr" << "is" << "it" << "nb" << "sv" << "zh_CN";
        QStringList choices;
        for (const QString &c : codes) choices << names.value(c, c);

        // Preselect the currently active language
        QString curLang = settings.value("language", QString()).toString();
        if (curLang.isEmpty()) curLang = "en_GB";
        int defaultIndex = codes.indexOf(curLang);
        if (defaultIndex < 0) defaultIndex = codes.indexOf("en_GB");

        // Custom dialog with translated buttons (Cancel/OK)
        QDialog dialog(nullptr);
        dialog.setWindowTitle(QObject::tr("Choose language"));
        QVBoxLayout* dlgLayout = new QVBoxLayout(&dialog);
        
        QLabel* dlgLabel = new QLabel(QObject::tr("Language:"));
        dlgLayout->addWidget(dlgLabel);
        
        QComboBox* dlgComboBox = new QComboBox();
        dlgComboBox->addItems(choices);
        dlgComboBox->setCurrentIndex(defaultIndex);
        dlgLayout->addWidget(dlgComboBox);
        
        QHBoxLayout* dlgButtonLayout = new QHBoxLayout();
        QPushButton* dlgOkButton = new QPushButton(QObject::tr("OK"));
        QPushButton* dlgCancelButton = new QPushButton(QObject::tr("Cancel"));
        dlgButtonLayout->addStretch();
        dlgButtonLayout->addWidget(dlgCancelButton);
        dlgButtonLayout->addWidget(dlgOkButton);
        dlgLayout->addLayout(dlgButtonLayout);
        
        QObject::connect(dlgOkButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(dlgCancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        bool ok = (dialog.exec() == QDialog::Accepted);
        QString pick = ok ? dlgComboBox->currentText() : QString();
        if (!ok || pick.isEmpty()) return;
        int idx = choices.indexOf(pick);
        QString code = (idx >= 0 && idx < codes.size()) ? codes.at(idx) : pick;
        // Normalize to an available translator before persisting/loading
        QString actual = normalizeLanguageCodeToAvailable(code);
        QString toWrite = actual.isEmpty() ? QString("en") : actual;
        settings.setValue("language", toWrite);
        settings.sync();  // Force write to disk
        appendLog(QString("i18n: Saved language '%1' to QSettings file: %2").arg(toWrite, settings.fileName()));
        appendLog(QString("i18n: QSettings status after save: %1").arg(settings.status() == QSettings::NoError ? "OK" : "ERROR"));

        // Apply the language change and preserve current tab selection.
        MultiRowTabWidget* oldTab = mainWindow.findChild<MultiRowTabWidget*>();
        int curIndex = oldTab ? oldTab->currentIndex() : -1;
        applyLanguage(toWrite, curIndex);

        QMessageBox::information(nullptr, QObject::tr("Language changed"), QObject::tr("Language saved. UI updated to the selected language."));
    });

    // Place Language button, then stretches around title, then About button on the right
    titleLayout->addWidget(langBtn);
    titleLayout->addStretch();
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
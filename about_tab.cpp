#include "about_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QTextEdit>
#include <QPixmap>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QWidget>
#include <QSizePolicy>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QTextStream>
#include <QDateTime>
#include "log_helper.h"
#include <QPushButton>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPair>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QTextBrowser>
#include "version.h"

namespace {
    // Try to detect the default browser desktop file via xdg-mime and
    // return a pair(program, args) to launch a new visible window if
    // possible. Returns an empty program on failure.
    static QPair<QString, QStringList> detectDefaultBrowserNewWindow()
    {
        QProcess p;
        p.start(QStringLiteral("xdg-mime"), QStringList() << QStringLiteral("query") << QStringLiteral("default") << QStringLiteral("x-scheme-handler/http"));
        if (!p.waitForFinished(500)) return {};
        QString desktopFile = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (desktopFile.isEmpty()) return {};

        // Map known desktop files to a command and initial args. We prefer
        // to try opening a new tab for browsers that support it (e.g. firefox)
        // and fall back to a new-window flag for others.
        QString prog;
        QStringList args;
        if (desktopFile.contains(QStringLiteral("firefox"), Qt::CaseInsensitive)) {
            prog = QStringLiteral("firefox");
            // We will try --new-tab first in the caller and then --new-window.
            args << QStringLiteral("--new-window");
        } else if (desktopFile.contains(QStringLiteral("chromium"), Qt::CaseInsensitive)) {
            prog = QStringLiteral("chromium");
            args << QStringLiteral("--new-window");
        } else if (desktopFile.contains(QStringLiteral("chrome"), Qt::CaseInsensitive) || desktopFile.contains(QStringLiteral("google-chrome"), Qt::CaseInsensitive)) {
            prog = QStringLiteral("google-chrome");
            args << QStringLiteral("--new-window");
        } else if (desktopFile.contains(QStringLiteral("opera"), Qt::CaseInsensitive)) {
            prog = QStringLiteral("opera");
            args << QStringLiteral("--new-window");
        }

        if (prog.isEmpty()) return {};
        return { prog, args };
    }

    // Check whether a helper program exists on PATH (fast, best-effort)
    static bool programExists(const QString &prog)
    {
        QProcess p;
        p.start(QStringLiteral("which"), QStringList() << prog);
        if (!p.waitForFinished(300)) return false;
        return p.exitCode() == 0;
    }

    // Best-effort: try to raise/activate any window(s) that belong to the
    // given pid. We prefer xdotool (search by pid) and fall back to wmctrl
    // (parse wmctrl -lp output). This is non-fatal — if the tools are not
    // present we log and continue. The function returns true if we managed
    // to activate any window.
    static bool attemptRaiseWindow(qint64 pid, const QString &link)
    {
        QFile logFile("/tmp/lsv-about-links.log");
        auto writeLog = [&](const QString &line) {
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream ts(&logFile);
                ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - " << line << "\n";
                logFile.close();
            }
        };

        // Try multiple attempts with exponential/backoff-like delays so
        // we can catch windows that are created slightly later by the
        // browser (some browsers spawn helper processes first).
        const int delaysMs[] = { 300, 700, 1400 };
        const int attempts = int(sizeof(delaysMs) / sizeof(delaysMs[0]));

        bool xdotoolAvail = programExists(QStringLiteral("xdotool"));
        bool wmctrlAvail = programExists(QStringLiteral("wmctrl"));
        if (!xdotoolAvail && !wmctrlAvail) {
            writeLog(QString("no raise tools installed (xdotool/wmctrl) for pid=%1 url=%2").arg(pid).arg(link));
            return false;
        }

        for (int attempt = 0; attempt < attempts; ++attempt) {
            QThread::msleep(delaysMs[attempt]);

            if (xdotoolAvail) {
                QProcess p;
                p.start(QStringLiteral("xdotool"), QStringList() << QStringLiteral("search") << QStringLiteral("--pid") << QString::number(pid));
                if (p.waitForFinished(800)) {
                    QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
                    if (!out.isEmpty()) {
                        const QStringList wins = out.split('\n', Qt::SkipEmptyParts);
                        for (const QString &w : wins) {
                            QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowactivate") << QStringLiteral("--sync") << w);
                            QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowraise") << w);
                            writeLog(QString("raised window via xdotool pid=%1 win=%2 url=%3 attempt=%4").arg(pid).arg(w).arg(link).arg(attempt));
                            return true;
                        }
                        writeLog(QString("xdotool no windows for pid=%1 url=%2 attempt=%3").arg(pid).arg(link).arg(attempt));
                    }
                } else {
                    writeLog(QString("xdotool search timed out for pid=%1 url=%2 attempt=%3").arg(pid).arg(link).arg(attempt));
                }
            }

            if (wmctrlAvail) {
                QProcess p;
                p.start(QStringLiteral("wmctrl"), QStringList() << QStringLiteral("-lp"));
                if (p.waitForFinished(800)) {
                    QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
                    const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
                    for (const QString &line : lines) {
                        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                        if (parts.size() >= 3) {
                            bool ok = false;
                            qint64 winPid = parts.at(2).toLongLong(&ok);
                            if (ok && winPid == pid) {
                                const QString winId = parts.at(0);
                                QProcess::startDetached(QStringLiteral("wmctrl"), QStringList() << QStringLiteral("-ia") << winId);
                                writeLog(QString("raised window via wmctrl pid=%1 win=%2 url=%3 attempt=%4").arg(pid).arg(winId).arg(link).arg(attempt));
                                return true;
                            }
                        }
                    }
                    writeLog(QString("wmctrl found no windows for pid=%1 url=%2 attempt=%3").arg(pid).arg(link).arg(attempt));
                } else {
                    writeLog(QString("wmctrl -lp timed out for pid=%1 url=%2 attempt=%3").arg(pid).arg(link).arg(attempt));
                }
            }
        }

        // If PID-based searches failed, try class/name based searches as a
        // last effort. Use xdotool --class and --name and wmctrl -lx.
        for (int attempt = 0; attempt < attempts; ++attempt) {
            QThread::msleep(delaysMs[attempt]);
            // xdotool: search by class (WM_CLASS) and by name (window title)
            if (xdotoolAvail) {
                // try class search (common names: Firefox, chromium, Google-chrome)
                QProcess p1;
                p1.start(QStringLiteral("xdotool"), QStringList() << QStringLiteral("search") << QStringLiteral("--class") << QStringLiteral("Firefox"));
                if (p1.waitForFinished(600)) {
                    QString out = QString::fromUtf8(p1.readAllStandardOutput()).trimmed();
                    if (!out.isEmpty()) {
                        const QStringList wins = out.split('\n', Qt::SkipEmptyParts);
                        for (const QString &w : wins) {
                            QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowactivate") << QStringLiteral("--sync") << w);
                            QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowraise") << w);
                            writeLog(QString("raised window via xdotool class=Firefox win=%1 url=%2 attempt=%3").arg(w).arg(link).arg(attempt));
                            return true;
                        }
                    }
                }
                // try name search using domain fragment
                QProcess p2;
                QString domainFrag;
                {
                    QUrl u(link);
                    domainFrag = u.host();
                }
                if (!domainFrag.isEmpty()) {
                    p2.start(QStringLiteral("xdotool"), QStringList() << QStringLiteral("search") << QStringLiteral("--name") << domainFrag);
                    if (p2.waitForFinished(600)) {
                        QString out = QString::fromUtf8(p2.readAllStandardOutput()).trimmed();
                        if (!out.isEmpty()) {
                            const QStringList wins = out.split('\n', Qt::SkipEmptyParts);
                            for (const QString &w : wins) {
                                QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowactivate") << QStringLiteral("--sync") << w);
                                QProcess::startDetached(QStringLiteral("xdotool"), QStringList() << QStringLiteral("windowraise") << w);
                                writeLog(QString("raised window via xdotool name-search win=%1 host=%2 url=%3 attempt=%4").arg(w).arg(domainFrag).arg(link).arg(attempt));
                                return true;
                            }
                        }
                    }
                }
            }

            if (wmctrlAvail) {
                QProcess p;
                p.start(QStringLiteral("wmctrl"), QStringList() << QStringLiteral("-lx"));
                if (p.waitForFinished(800)) {
                    QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
                    const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
                    for (const QString &line : lines) {
                        // wmctrl -lx columns: WINID DESKTOP WM_CLASS HOST TITLE
                        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                        if (parts.size() >= 5) {
                            const QString winId = parts.at(0);
                            const QString wmClass = parts.at(2);
                            const QString title = parts.mid(4).join(' ');
                            // match class or title containing host fragment
                            QUrl u(link);
                            QString host = u.host();
                            if (wmClass.contains(QStringLiteral("firefox"), Qt::CaseInsensitive) || wmClass.contains(QStringLiteral("chrome"), Qt::CaseInsensitive) || (!host.isEmpty() && title.contains(host, Qt::CaseInsensitive))) {
                                QProcess::startDetached(QStringLiteral("wmctrl"), QStringList() << QStringLiteral("-ia") << winId);
                                writeLog(QString("raised window via wmctrl match win=%1 wmclass=%2 title=%3 url=%4 attempt=%5").arg(winId).arg(wmClass).arg(title).arg(link).arg(attempt));
                                return true;
                            }
                        }
                    }
                }
            }
        }

        writeLog(QString("raise attempts exhausted for pid=%1 url=%2").arg(pid).arg(link));
        return false;
    }

    // Non-blocking starter: run attemptRaiseWindow in a background thread so
    // the GUI thread isn't blocked by sleeps/waits. This returns immediately.
    static void startRaiseAsync(qint64 pid, const QString &link)
    {
        if (pid <= 0) return;
        QFuture<void> f = QtConcurrent::run([pid, link]() {
            attemptRaiseWindow(pid, link);
        });
        Q_UNUSED(f);
    }
    // Try to open a URL using QDesktopServices first, then fall back to
    // a list of common desktop helpers. We keep this in an anonymous
    // namespace to limit visibility to this translation unit.
    static void openUrlRobust(const QString &link)
    {
        // First try to launch the system default browser with a "new-window"
        // flag if we can detect it. This helps ensure a visible window/tab is
        // opened for portable AppImage users (some browsers open a tab in an
        // existing session without raising the window).
        {
            auto brush = detectDefaultBrowserNewWindow();
            const QString &prog = brush.first;
            QStringList baseArgs = brush.second;
            if (!prog.isEmpty()) {
                qDebug() << "openUrlRobust: detected default browser program:" << prog << "baseArgs=" << baseArgs;
                // For Firefox prefer --new-tab first, then fall back to --new-window.
                QList<QStringList> variants;
                if (prog.contains(QStringLiteral("firefox"), Qt::CaseInsensitive)) {
                    variants << (QStringList() << QStringLiteral("--new-tab") << link);
                    QStringList v2 = baseArgs;
                    v2 << link;
                    variants << v2;
                } else {
                    QStringList v = baseArgs;
                    v << link;
                    variants << v;
                }

                for (const QStringList &args : variants) {
                    qint64 pid = 0;
                    qDebug() << "openUrlRobust: trying default browser launch:" << prog << args;
                    bool started = QProcess::startDetached(prog, args, QString(), &pid);
                    QFile logFile("/tmp/lsv-about-links.log");
                    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                        QTextStream ts(&logFile);
                        ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - attempted default-browser launch: " << prog << " args=" << args.join(' ') << " url=" << link;
                        if (started) ts << " pid=" << pid;
                        ts << "\n";
                        logFile.close();
                    }
                    if (started) {
                        // Best-effort: try to raise the newly-created window/tab in
                        // the background so the GUI doesn't block. Return now as
                        // the launch succeeded.
                        startRaiseAsync(pid, link);
                        return;
                    }
                }
            }
        }
        QUrl url(link);
        if (QDesktopServices::openUrl(url)) {
            qDebug() << "openUrlRobust: QDesktopServices succeeded for" << link;
            // Log to a persistent file so we can diagnose launches from file managers
            QFile logFile("/tmp/lsv-about-links.log");
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream ts(&logFile);
                ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - opened via QDesktopServices: " << link << "\n";
                logFile.close();
            }
            return;
        }

        qDebug() << "openUrlRobust: QDesktopServices failed for" << link << "; trying fallbacks";

        // Common fallbacks on various desktop environments
        const QStringList fallbackCmds = {
            QStringLiteral("xdg-open"),
            QStringLiteral("gio open"),
            QStringLiteral("gnome-open"),
            QStringLiteral("kde-open5"),
            QStringLiteral("sensible-browser"),
            QStringLiteral("x-www-browser")
        };

    for (const QString &cmd : fallbackCmds) {
            // Some entries contain a space (e.g. "gio open") — split them.
            const QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
            const QString program = parts.value(0);
            QStringList args;
            if (parts.size() > 1) {
                // append the rest of the parts as initial args followed by the URL
                for (int i = 1; i < parts.size(); ++i) args << parts.at(i);
            }
            args << link;
            qDebug() << "openUrlRobust: trying" << program << args;
            qint64 pid = 0;
            bool started = QProcess::startDetached(program, args, QString(), &pid);
                if (started) {
                qDebug() << "openUrlRobust: started" << program << "for" << link << "pid=" << pid;
                QFile logFile("/tmp/lsv-about-links.log");
                if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                    QTextStream ts(&logFile);
                    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - started fallback: " << program << " args=" << args.join(' ') << " pid=" << pid << " url=" << link << "\n";
                    logFile.close();
                }
                // Try to raise the window/tab in background; return since the
                // fallback launcher was started successfully.
                startRaiseAsync(pid, link);
                return;
            } else {
                QFile logFile("/tmp/lsv-about-links.log");
                if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                    QTextStream ts(&logFile);
                    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - fallback failed: " << program << " args=" << args.join(' ') << " url=" << link << "\n";
                    logFile.close();
                }
            }
        }

        // As a last attempt run the command through the shell. This can
        // succeed in constrained environments where direct exec fails.
        QString shellCmd = QStringLiteral("xdg-open %1").arg(link);
        qint64 shellPid = 0;
        bool shellStarted = QProcess::startDetached(QStringLiteral("sh"), QStringList() << QStringLiteral("-c") << shellCmd, QString(), &shellPid);
            if (shellStarted) {
            qDebug() << "openUrlRobust: started shell fallback for" << link << "pid=" << shellPid;
            QFile logFile("/tmp/lsv-about-links.log");
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream ts(&logFile);
                ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - started shell fallback: " << shellCmd << " pid=" << shellPid << " url=" << link << "\n";
                logFile.close();
            }
            // Try to raise the shell-launched browser window in background
            // (non-blocking) and return since the shell fallback was started.
            startRaiseAsync(shellPid, link);
            return;
        } else {
            QFile logFile("/tmp/lsv-about-links.log");
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream ts(&logFile);
                ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - shell fallback failed: " << shellCmd << " url=" << link << "\n";
                logFile.close();
            }
        }
        qDebug() << "openUrlRobust: shell fallback failed for" << link << "; trying direct browser executables";

        // Final attempt: try launching common browser binaries directly.
        const QStringList browsers = { QStringLiteral("firefox"), QStringLiteral("chromium"), QStringLiteral("google-chrome"), QStringLiteral("brave-browser"), QStringLiteral("chrome") };
        for (const QString &browser : browsers) {
            qDebug() << "openUrlRobust: trying browser" << browser << link;
            // Prefer new-tab for firefox, otherwise try new-window then plain invocation
            QList<QStringList> attempts;
            if (browser.contains(QStringLiteral("firefox"), Qt::CaseInsensitive)) {
                attempts << (QStringList() << QStringLiteral("--new-tab") << link);
                attempts << (QStringList() << QStringLiteral("--new-window") << link);
            } else {
                attempts << (QStringList() << QStringLiteral("--new-window") << link);
                attempts << (QStringList() << link);
            }
            for (const QStringList &args : attempts) {
                qint64 pid = 0;
                bool started = QProcess::startDetached(browser, args, QString(), &pid);
                if (started) {
                    qDebug() << "openUrlRobust: started browser" << browser << "for" << link << "pid=" << pid;
                    QFile logFile("/tmp/lsv-about-links.log");
                    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                        QTextStream ts(&logFile);
                        ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - started browser: " << browser << " args=" << args.join(' ') << " pid=" << pid << " url=" << link << "\n";
                        logFile.close();
                    }
                    // Try to raise the newly-started browser window. If the
                    // raise worked, we're done; otherwise continue with other
                    // attempts.
                    startRaiseAsync(pid, link);
                    return;
                } else {
                    QFile logFile("/tmp/lsv-about-links.log");
                    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                        QTextStream ts(&logFile);
                        ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - browser attempt failed: " << browser << " args=" << args.join(' ') << " url=" << link << "\n";
                        logFile.close();
                    }
                }
            }
        }

        qDebug() << "openUrlRobust: all attempts to open URL failed:" << link;
        QFile logFile("/tmp/lsv-about-links.log");
        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&logFile);
            ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " - failed to open url: " << link << "\n";
            logFile.close();
        }
    }
}

AboutTab::AboutTab(QWidget* parent)
    : TabWidgetBase("About", "echo 'Linux System Viewer V. " LSV_VERSION "'", false, "", parent)
{
    qDebug() << "AboutTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "AboutTab: Constructor finished";
}

QWidget* AboutTab::createUserFriendlyView()
{
    qDebug() << "AboutTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Remove default scroll area margins so the content sits at the top
    scrollArea->setContentsMargins(0, 0, 0, 0);
    // Align content to the top so the header isn't vertically centered
    scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    
    QWidget* contentWidget = new QWidget();
    // Make the about content wider by default, but not excessive.
    // Previously we used 1.5x; make the content 75% of that now (1.125x)
    // If the tab has a sensible width, use that as a base; otherwise fall back to 900px.
    int baseWidth = 900;
    if (this && this->width() > 100) {
        baseWidth = this->width();
    }
    int targetWidth = int(baseWidth * 1.125); // 75% of previous 1.5 multiplier
    contentWidget->setMinimumWidth(targetWidth);
    contentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    // Make the scroll area and this tab request a matching minimum so the window can expand
    scrollArea->setMinimumWidth(targetWidth + 24);
    scrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    this->setMinimumWidth(targetWidth + 48);
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    // Reduce overall spacing and top margin so the header (logo + title)
    // appears compact and the headline isn't pushed down too far.
    mainLayout->setSpacing(10);
    // Remove top margin entirely to eliminate the empty space above the logo
    mainLayout->setContentsMargins(20, 0, 20, 20);
    
    // Application logo and title
    QLabel* logoLabel = new QLabel();
    // Prefer a workspace lsv.png if present so local icon edits appear instantly
    QString localLogoPath = QDir::currentPath() + QDir::separator() + "lsv.png";
    QPixmap logo;
    if (QFile::exists(localLogoPath)) {
        logo.load(localLogoPath);
    }
    if (logo.isNull()) {
        // Fallback to embedded resource (resources.qrc contains 'lsv.png')
        logo.load(":/lsv.png");
    }
    if (!logo.isNull()) {
        // Increase the icon size by 60% (96 -> ~154) to match the user's request
        const int baseLogoSize = 96;
        const int increasedSize = int(baseLogoSize * 1.6); // ~153.6 -> 154
        logoLabel->setPixmap(logo.scaled(increasedSize, increasedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        // Make the label request the new size so layout respects it
        logoLabel->setFixedSize(increasedSize, increasedSize);
    } else {
        // If no image is available, fall back to a styled placeholder sized to the increased size
        const int baseLogoSize = 96;
        const int increasedSize = int(baseLogoSize * 1.6);
        logoLabel->setStyleSheet(
            "QLabel {"
            "  font-size: 36px;"
            "  font-weight: bold;"
            "  color: #2c3e50;"
            "  background-color: #ecf0f1;"
            "  border-radius: 76px;"
            "  min-width: 154px;"
            "  min-height: 154px;"
            "}"
        );
        logoLabel->setMinimumSize(increasedSize, increasedSize);
    }
    // Make the logo and title a compact header so there's no large gap
    // between them and the rest of the content.
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel* titleLabel = new QLabel("Linux System Viewer");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 24px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin: 6px 0 12px 0;"  // smaller top/bottom margins
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(2);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(logoLabel, 0, Qt::AlignCenter);
    headerLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addLayout(headerLayout);
    
    createInfoSection("Application", &m_applicationSection, &m_applicationContent, mainLayout);
    createInfoSection("Version", &m_versionSection, &m_versionContent, mainLayout);
    createInfoSection("Authors", &m_authorsSection, &m_authorsContent, mainLayout);
    // Clicking the authors web page link is handled by QLabel::linkActivated
    // which calls openUrlRobust(). We removed the explicit Open/Copy buttons
    // to focus on making the QLabel links behave correctly in file-manager
    // launches (the robust open + raise logic should handle visible opens).
    createInfoSection("License", &m_licenseSection, &m_licenseContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);

    // Force the About tab and scroll area to request a 450px minimum height per user request
    const int forcedHeight = 450;
    scrollArea->setMinimumHeight(forcedHeight);
    contentWidget->setMinimumHeight(forcedHeight - 24);
    this->setMinimumHeight(forcedHeight);

    // Request layout updates so the parent window can adjust to the new preferred size
    contentWidget->updateGeometry();
    scrollArea->updateGeometry();
    this->updateGeometry();
    this->adjustSize();
    
    qDebug() << "AboutTab: createUserFriendlyView completed";
    return scrollArea;
}

void AboutTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel();
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 15px; background-color: #f8f9fa; border-radius: 4px; line-height: 1.4; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void AboutTab::parseOutput(const QString& output)
{
    qDebug() << "AboutTab: parseOutput called";
    
    // Static information for About tab
    QString applicationInfo = 
        "Linux System Viewer is a comprehensive system information tool "
        "designed to provide detailed insights into your Linux system hardware "
        "and software configuration.\n\n"
        "Linux System Viewer presents system information in an intuitive, easy-to-read format "
        "with both user-friendly and technical (geek mode) views for different "
        "levels of detail.";
    
    QString versionInfo = QStringLiteral("Version: %1\nBuild Date: October 2025\nQt Version: %2\nPlatform: Linux")
                          .arg(LSVVersionQString(), QString::fromUtf8(QT_VERSION_STR));
    
    QString authorsInfo =
        "Developer: Nalle Berg<br>"
        "<a href=\"https://lsv.nalle.no/\">Web page</a><br><br>"
        "Built with Qt6 and modern C++ for optimal performance "
        "and cross-platform compatibility.<br><br>"
    "Special thanks to the open-source community and the "
    "developers of lshw, lscpu, and other system utilities "
    "that inspired me to create this application.";
    
    // Show a simple clickable license link (rendered as HTML)
    QString licenseInfo =
        "<a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.html\">GPL V2</a>";
    
    // Update the UI with static information
    if (m_applicationContent) {
        m_applicationContent->setText(applicationInfo);
    }
    if (m_versionContent) {
        m_versionContent->setText(versionInfo);
    }
    if (m_authorsContent) {
        // Render the authors section as rich text so the Web page link is clickable.
        // Do not let Qt open links directly; handle them explicitly so our
        // robust fallback logic always runs when a user clicks a link and
        // so we can log the click via appendLog.
        m_authorsContent->setTextFormat(Qt::RichText);
        m_authorsContent->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
        m_authorsContent->setOpenExternalLinks(false);
        m_authorsContent->setText(authorsInfo);
        connect(m_authorsContent, &QLabel::linkActivated, this, [this](const QString &link) {
            appendLog(QString("About: authors link clicked: %1").arg(link));
            // For the authors web page we show a copy-to-clipboard dialog
            // instead of launching a browser from the AppImage. This keeps
            // the application read-only while still letting the user open
            // the page manually if they wish.
            if (link.contains(QStringLiteral("lsv.nalle.no"), Qt::CaseInsensitive)) {
                QDialog dlg(this);
                dlg.setWindowTitle(QStringLiteral("Open Web Page"));
                QVBoxLayout *lay = new QVBoxLayout(&dlg);
                QLabel *msg = new QLabel(&dlg);
                msg->setWordWrap(true);
                msg->setText(QStringLiteral(
                    "This is a read only application. For security reasons this app will not do anything to your disk nor start any applications.\n\n"
                    "However click below to copy the URL https://lsv.nalle.no/ to the clipboard."));
                lay->addWidget(msg);

                QDialogButtonBox *box = new QDialogButtonBox(&dlg);
                QPushButton *copyBtn = box->addButton(QStringLiteral("Copy URL"), QDialogButtonBox::ActionRole);
                QPushButton *closeBtn = box->addButton(QDialogButtonBox::Close);
                lay->addWidget(box);

                connect(copyBtn, &QPushButton::clicked, &dlg, [this, link, &dlg]() {
                    QGuiApplication::clipboard()->setText(link);
                    // Close the dialog first, then show confirmation so the
                    // confirmation appears on top of the main window rather
                    // than being nested inside the about dialog.
                    dlg.accept();
                    // Use a single-shot to let the dialog close and the event
                    // loop process the close before we show the message box.
                    QTimer::singleShot(0, this, [this]() {
                        QMessageBox::information(nullptr, QStringLiteral("Copied"), QStringLiteral("The URL was copied to the clipboard!"));
                    });
                });
                connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

                dlg.exec();
            } else {
                openUrlRobust(link);
            }
        });
    }
    if (m_licenseContent) {
        // Use rich text and handle link activation explicitly so our
        // robust fallback logic runs and we record the click. For the
        // GPLv2 link we prefer to show the embedded license shipped with
        // the AppImage (:/gpl2.txt) so users can read it offline.
        m_licenseContent->setTextFormat(Qt::RichText);
        m_licenseContent->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
        m_licenseContent->setOpenExternalLinks(false);
        m_licenseContent->setText(licenseInfo);
        connect(m_licenseContent, &QLabel::linkActivated, this, [this](const QString &link) {
            appendLog(QString("About: license link clicked: %1").arg(link));
            // If the clicked link points to the GPLv2 page, show the
            // embedded license text instead of opening the external URL.
            if (link.contains(QStringLiteral("gnu.org/licenses")) || link.contains(QStringLiteral("gpl-2.0"))) {
                // Load embedded license text from resources and show in dialog
                QFile f(QStringLiteral(":/gpl2.txt"));
                QString text;
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    text = QString::fromUtf8(f.readAll());
                    f.close();
                } else {
                    text = QStringLiteral("(Embedded license not found)");
                }
                QDialog dlg(this);
                dlg.setWindowTitle(QStringLiteral("GNU GPL v2"));
                QVBoxLayout *lay = new QVBoxLayout(&dlg);

                // Use the embedded GNU icon resource only (ensure it's present
                // in resources.qrc so the AppImage always contains it).
                QPixmap gnuPix;
                if (QFile::exists(QStringLiteral(":/gnu_icon.png"))) {
                    gnuPix.load(QStringLiteral(":/gnu_icon.png"));
                }
                if (!gnuPix.isNull()) {
                    // Limit displayed width so it fits nicely; preserve aspect ratio.
                    const int maxWidth = 128;
                    QPixmap scaled = gnuPix.scaledToWidth(maxWidth, Qt::SmoothTransformation);
                    QLabel *iconLabel = new QLabel(&dlg);
                    iconLabel->setPixmap(scaled);
                    iconLabel->setAlignment(Qt::AlignCenter);
                    // Add with center alignment so it's visually on top of the text
                    lay->addWidget(iconLabel, 0, Qt::AlignHCenter);
                }
                // Build a simple HTML rendering of the license that highlights
                // section headings (e.g. "Preamble") and uses a nice sans font
                // for headings and a monospaced font for the license body.
                auto escapeHtml = [](const QString &s) {
                    QString out = s;
                    out.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
                    out.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
                    out.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
                    return out;
                };

                // Normalize line endings
                QString raw = text;
                raw.replace("\r\n", "\n");
                raw.replace('\r', '\n');

                // Split into lines and detect simple headings: either the word
                // "Preamble" or lines that are mostly uppercase (and not too
                // short). We'll render each heading as a mini-headline.
                const QStringList lines = raw.split('\n');
                struct Section { QString heading; QString body; };
                QVector<Section> sections;
                Section cur; // start with no heading
                for (int i = 0; i < lines.size(); ++i) {
                    const QString ln = lines.at(i).trimmed();
                    bool isHeading = false;
                    if (ln.compare(QStringLiteral("Preamble"), Qt::CaseInsensitive) == 0) isHeading = true;
                    // all-letters and spaces and punctuation but uppercase -> heading
                    else if (ln.length() > 3) {
                        QString lettersOnly;
                        for (QChar c : ln) if (c.isLetter() || c.isSpace()) lettersOnly.append(c);
                        if (!lettersOnly.isEmpty() && lettersOnly == lettersOnly.toUpper()) isHeading = true;
                    }

                    if (isHeading) {
                        // start a new section
                        if (!cur.heading.isEmpty() || !cur.body.isEmpty()) sections.append(cur);
                        cur.heading = ln;
                        cur.body.clear();
                    } else {
                        if (!cur.body.isEmpty()) cur.body.append('\n');
                        cur.body.append(lines.at(i));
                    }
                }
                if (!cur.heading.isEmpty() || !cur.body.isEmpty()) sections.append(cur);

                // Compose HTML
                QString html;
                html += QStringLiteral("<div style='font-family:Arial, Helvetica, sans-serif; color:#2c3e50;'>");
                // Main headline centered and larger
                html += QStringLiteral("<div style='text-align:center; font-size:20pt; font-weight:bold; margin-bottom:8px;'>GNU General Public License v2</div>");
                // Optional icon is already added above; now render sections
                for (const Section &s : sections) {
                    if (!s.heading.isEmpty()) {
                        html += QStringLiteral("<div style='font-size:13pt; font-weight:bold; margin-top:12px; margin-bottom:6px; color:#1f618d;'>%1</div>").arg(escapeHtml(s.heading));
                    }
                    // body: escape then preserve newlines via <pre> (allow wrap)
                    html += QStringLiteral("<div style='font-family:monospace; font-size:10pt; white-space:pre-wrap; color:#222;'>") + escapeHtml(s.body) + QStringLiteral("</div>");
                }
                html += QStringLiteral("</div>");

                QTextBrowser *tb = new QTextBrowser(&dlg);
                tb->setReadOnly(true);
                tb->setOpenExternalLinks(false);
                tb->setHtml(html);
                tb->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
                lay->addWidget(tb);
                QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
                connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                lay->addWidget(box);
                dlg.resize(780, 560);
                dlg.exec();
            } else {
                openUrlRobust(link);
            }
        });
    }
    
    qDebug() << "AboutTab: parseOutput completed";
}
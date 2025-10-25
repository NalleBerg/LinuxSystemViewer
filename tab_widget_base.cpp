#include "tab_widget_base.h"
#include <QDebug>
#include "log_helper.h"
#include <QLabel>
#include <QMovie>
#include <QApplication>

TabWidgetBase::TabWidgetBase(const QString& tabName, const QString& command, 
                            bool hasGeekMode, const QString& geekCommand, 
                            QWidget* parent)
    : QWidget(parent)
    , m_tabName(tabName)
    , m_command(command)
    , m_hasGeekMode(hasGeekMode)
    , m_geekCommand(geekCommand)
    , m_process(nullptr)
    , m_isLoading(false)
{
    setupUI();
}

void TabWidgetBase::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget();
    m_mainLayout->addWidget(m_stackedWidget);

    m_loadingWidget = new QWidget();
    QVBoxLayout* loadingLayout = new QVBoxLayout(m_loadingWidget);
    loadingLayout->setAlignment(Qt::AlignCenter);

    m_loadingLabel = new QLabel("Loading system information...");
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 16px;"
        "  color: #666;"
        "  padding: 20px;"
        "}"
    );
    loadingLayout->addWidget(m_loadingLabel);

    m_stackedWidget->addWidget(m_loadingWidget);
}

void TabWidgetBase::initializeTab()
{
    m_userFriendlyWidget = createUserFriendlyView();
    if (m_userFriendlyWidget) {
        m_stackedWidget->addWidget(m_userFriendlyWidget);
    }
    executeCommand();
}

void TabWidgetBase::refreshData()
{
    executeCommand();
}

void TabWidgetBase::executeCommand()
{
    if (m_isLoading)
        return;

    // Always run a command, even if it's a dummy one
    if (m_command.isEmpty()) {
        m_command = "true"; // fallback to dummy command
    }

    showLoadingMessage();
    emit loadingStarted();

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TabWidgetBase::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &TabWidgetBase::onProcessError);
    connect(m_process, &QProcess::started, this, [this]() {
        appendLog(QString("TabWidgetBase: Process started for %1 command: %2").arg(m_tabName, m_command));
        appendLog(QString("ENV PATH: %1").arg(QString::fromLocal8Bit(qgetenv("PATH"))));
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString("TabWidgetBase: readyReadStandardOutput for %1").arg(m_tabName));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString("TabWidgetBase: readyReadStandardError for %1").arg(m_tabName));
    });

    m_process->start("bash", QStringList() << "-c" << m_command);
    m_isLoading = true;
}

void TabWidgetBase::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_process) {
        QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        QString errorOutput = QString::fromLocal8Bit(m_process->readAllStandardError());

        // Filter known noisy warnings (e.g., lshw warning about super-user) before logging stderr
        auto filterStderr = [](const QString& in) {
            if (in.trimmed().isEmpty()) return QString();
            QStringList lines = in.split('\n');
            QStringList out;
            for (const QString& l : lines) {
                QString t = l.trimmed();
                // ignore lshw warnings about running as super-user
                if (t.contains("you should run this program as super-user", Qt::CaseInsensitive)) continue;
                if (t.contains("output may be incomplete or inaccurate", Qt::CaseInsensitive)) continue;
                out << l;
            }
            return out.join('\n').trimmed();
        };

        QString filteredErr = filterStderr(errorOutput);

        appendLog(QString("TabWidgetBase: Process finished for %1 exitCode: %2 exitStatus: %3").arg(m_tabName).arg(exitCode).arg((int)exitStatus));
        appendLog(QString("TabWidgetBase: Output length: %1 Err length: %2").arg(QString::number(output.size())).arg(QString::number(filteredErr.size())));
        if (!filteredErr.isEmpty()) appendLog(QString("TabWidgetBase: Process stderr: %1").arg(filteredErr));

        m_lastOutput = output;
        parseOutput(output);

        m_process->deleteLater();
        m_process = nullptr;
    }

    hideLoadingMessage();
    emit loadingFinished();
    m_isLoading = false;
}

void TabWidgetBase::onProcessError(QProcess::ProcessError error)
{
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_loadingLabel->setText(QString("Error loading %1 information").arg(m_tabName));
    emit loadingFinished();
    m_isLoading = false;
}

void TabWidgetBase::showLoadingMessage()
{
    m_loadingLabel->setText(QString("Loading %1 information...").arg(m_tabName));
    m_stackedWidget->setCurrentWidget(m_loadingWidget);
}

void TabWidgetBase::hideLoadingMessage()
{
    if (m_userFriendlyWidget) {
        m_stackedWidget->setCurrentWidget(m_userFriendlyWidget);
    }
}
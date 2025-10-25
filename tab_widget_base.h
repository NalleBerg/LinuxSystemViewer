#ifndef TAB_WIDGET_BASE_H
#define TAB_WIDGET_BASE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QProcess>

class TabWidgetBase : public QWidget
{
    Q_OBJECT
public:
    explicit TabWidgetBase(const QString& tabName,
                           const QString& command,
                           bool hasGeekMode,
                           const QString& geekCommand,
                           QWidget* parent = nullptr);
    virtual ~TabWidgetBase() = default;

    QString getTabName() const { return m_tabName; }
    void refreshData();

signals:
    void loadingStarted();
    void loadingFinished();

protected:
    virtual QWidget* createUserFriendlyView() = 0;
    virtual void parseOutput(const QString& output) = 0;

    void initializeTab();
    void executeCommand();

    // Members for derived classes
    QString m_tabName;
    QString m_command;
    bool m_showHeader;
    QString m_headerText;
    QString m_lastOutput;

    // Geek mode members
    bool m_hasGeekMode;
    QString m_geekCommand;

    // UI members
    QVBoxLayout* m_mainLayout;
    QStackedWidget* m_stackedWidget;
    QWidget* m_userFriendlyWidget;
    QWidget* m_loadingWidget;
    QLabel* m_loadingLabel;

    QProcess* m_process;
    bool m_isLoading;

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setupUI();
    void showLoadingMessage();
    void hideLoadingMessage();
};

#endif // TAB_WIDGET_BASE_H
#include "elevation_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QStyle>
#include <QProcess>
#include <QCoreApplication>
#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTranslator>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <unistd.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <cstring>

// PAM conversation function - used to provide password to PAM
static int pamConversation(int num_msg, const struct pam_message **msg,
                          struct pam_response **resp, void *appdata_ptr)
{
    if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG)
        return PAM_CONV_ERR;

    // Get the password from appdata
    const char *password = static_cast<const char *>(appdata_ptr);
    
    *resp = static_cast<struct pam_response *>(calloc(num_msg, sizeof(struct pam_response)));
    if (*resp == nullptr)
        return PAM_BUF_ERR;

    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            (*resp)[i].resp = strdup(password);
            (*resp)[i].resp_retcode = 0;
        }
    }

    return PAM_SUCCESS;
}

ElevationDialog::ElevationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Authentication Required"));
    setModal(true);
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Icon and message at top
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    // Security shield icon
    QLabel *iconLabel = new QLabel();
    QIcon securityIcon = style()->standardIcon(QStyle::SP_MessageBoxQuestion);
    iconLabel->setPixmap(securityIcon.pixmap(48, 48));
    headerLayout->addWidget(iconLabel);
    
    // Message
    m_messageLabel = new QLabel();
    m_messageLabel->setText(tr("<b>Authentication is required to run Linux System Viewer</b><br><br>"
                              "Linux System Viewer requires administrative privileges to access "
                              "hardware information. Please enter your password to continue."));
    m_messageLabel->setWordWrap(true);
    headerLayout->addWidget(m_messageLabel, 1);
    
    mainLayout->addLayout(headerLayout);

    // Username display
    struct passwd *pw = getpwuid(getuid());
    QString username = pw ? QString::fromLocal8Bit(pw->pw_name) : "user";
    
    QLabel *userLabel = new QLabel(tr("User: <b>%1</b>").arg(username));
    mainLayout->addWidget(userLabel);

    // Password field
    QLabel *passwordLabel = new QLabel(tr("Password:"));
    mainLayout->addWidget(passwordLabel);
    
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Enter your password"));
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &ElevationDialog::onAuthenticateClicked);
    mainLayout->addWidget(m_passwordEdit);

    // Error label (hidden by default)
    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet("QLabel { color: red; }");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);
    
    // Hide error message when user starts typing (must be after m_errorLabel is created)
    connect(m_passwordEdit, &QLineEdit::textChanged, m_errorLabel, &QLabel::hide);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    
    m_authenticateButton = new QPushButton(tr("Authenticate"));
    m_authenticateButton->setDefault(true);
    connect(m_authenticateButton, &QPushButton::clicked, this, &ElevationDialog::onAuthenticateClicked);
    buttonLayout->addWidget(m_authenticateButton);
    
    mainLayout->addLayout(buttonLayout);

    // Focus password field
    m_passwordEdit->setFocus();
}

ElevationDialog::~ElevationDialog()
{
}

bool ElevationDialog::authenticateWithPAM(const QString &username, const QString &password)
{
    pam_handle_t *pamh = nullptr;
    
    // Prepare PAM conversation
    QByteArray passwordBytes = password.toLocal8Bit();
    struct pam_conv conv = {
        pamConversation,
        passwordBytes.data()
    };

    // Start PAM
    QByteArray usernameBytes = username.toLocal8Bit();
    int retval = pam_start("sudo", usernameBytes.constData(), &conv, &pamh);
    if (retval != PAM_SUCCESS) {
        if (pamh)
            pam_end(pamh, retval);
        return false;
    }

    // Authenticate
    retval = pam_authenticate(pamh, 0);
    if (retval != PAM_SUCCESS) {
        pam_end(pamh, retval);
        return false;
    }

    // Validate account
    retval = pam_acct_mgmt(pamh, 0);
    if (retval != PAM_SUCCESS) {
        pam_end(pamh, retval);
        return false;
    }

    // Clean up
    pam_end(pamh, PAM_SUCCESS);
    
    // Clear password from memory
    passwordBytes.fill('0');
    
    return true;
}

bool ElevationDialog::restartAsRoot(const QString &password)
{
    // Get current executable path
    // Always use the actual running binary for elevation
    QString exe = QCoreApplication::applicationFilePath();
    
    // Prepare environment variables to preserve display and user home
    QString display = QString::fromLocal8Bit(qgetenv("DISPLAY"));
    QString xdgSession = QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE"));
    QString waylandDisplay = QString::fromLocal8Bit(qgetenv("WAYLAND_DISPLAY"));
    QString xauthority = QString::fromLocal8Bit(qgetenv("XAUTHORITY"));
    QString xdgRuntimeDir = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
    QString userHome = QString::fromLocal8Bit(qgetenv("HOME"));  // Preserve original user's HOME
    
    // Create a shell script that will run sudo and start the elevated process
    QString scriptPath = QDir::tempPath() + QDir::separator() + 
                        QString("lsv-elevate-%1.sh").arg(QCoreApplication::applicationPid());
    
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&script);
    out << "#!/bin/bash\n";
    out << "# Temporary elevation script - removes itself on exit\n";
    out << "trap 'rm -f \"" << scriptPath << "\"' EXIT\n\n";

    // Escape password for shell
    QString escapedPassword = password;
    escapedPassword.replace("'", "'\\''");

    // Write password to sudo via pipe and start elevated instance
    out << "echo '" << escapedPassword << "' | sudo -S ";

    // Preserve environment variables
    if (!display.isEmpty())
        out << "DISPLAY='" << display.replace("'", "'\\''") << "' ";
    if (!xauthority.isEmpty())
        out << "XAUTHORITY='" << xauthority.replace("'", "'\\''") << "' ";
    if (!xdgRuntimeDir.isEmpty())
        out << "XDG_RUNTIME_DIR='" << xdgRuntimeDir.replace("'", "'\\''") << "' ";
    if (!xdgSession.isEmpty())
        out << "XDG_SESSION_TYPE='" << xdgSession.replace("'", "'\\''") << "' ";
    if (!waylandDisplay.isEmpty())
        out << "WAYLAND_DISPLAY='" << waylandDisplay.replace("'", "'\\''") << "' ";
    if (!userHome.isEmpty())
        out << "HOME='" << userHome.replace("'", "'\\''") << "' ";  // Keep user's HOME, not root's

    // Set LSV_ELEVATED flag and export user config directory (not .conf file)
    QString lsvUserConfig = QDir::homePath() + "/.config/LinuxSystemViewer/LSV.conf";
    // Debug: log environment and command to a file in /home/nalle/Dokumenter/c++/lshwgui/logs/
    out << "mkdir -p '/home/nalle/Dokumenter/c++/lshwgui/logs'\n";
    out << "env > '/home/nalle/Dokumenter/c++/lshwgui/logs/lsv_elevated_env.log'\n";
    out << "echo 'Launching: echo <password> | sudo -S env LSV_ELEVATED=1 LSV_USER_CONFIG='" << lsvUserConfig.replace("'", "'\\''") << "' setsid '" << exe.replace("'", "'\\''") << "' HOME='" << userHome.replace("'", "'\\''") << "'' > '/home/nalle/Dokumenter/c++/lshwgui/logs/lsv_elevated_cmd.log'\n";
    // Start the application in the background with setsid, preserving environment, and non-interactive sudo
    out << "echo '" << escapedPassword << "' | sudo -S env LSV_ELEVATED=1 LSV_USER_CONFIG='" << lsvUserConfig.replace("'", "'\\''") << "' HOME='" << userHome.replace("'", "'\\''") << "' setsid '" << exe.replace("'", "'\\''") << "' > '/home/nalle/Dokumenter/c++/lshwgui/logs/lsv_elevated_out.log' 2>&1 &\n";

    script.close();

    // Make script executable
    QFile::setPermissions(scriptPath, QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
    
    // Execute the script in detached mode
    bool started = QProcess::startDetached("/bin/bash", QStringList() << scriptPath);
    
    if (started) {
        // Give it a moment to start
        QThread::msleep(500);
        return true;
    }
    
    // Clean up if failed
    QFile::remove(scriptPath);
    return false;
}

void ElevationDialog::onAuthenticateClicked()
{
    QString password = m_passwordEdit->text();
    
    if (password.isEmpty()) {
        m_errorLabel->setText(tr("Password cannot be empty"));
        m_errorLabel->show();
        return;
    }
    
    // Disable UI during authentication
    m_passwordEdit->setEnabled(false);
    m_authenticateButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    m_authenticateButton->setText(tr("Authenticating..."));
    
    // Get username
    struct passwd *pw = getpwuid(getuid());
    QString username = pw ? QString::fromLocal8Bit(pw->pw_name) : "user";
    
    // Try PAM authentication
    if (authenticateWithPAM(username, password)) {
        // Authentication successful, restart as root
        if (restartAsRoot(password)) {
            // Password cleared in restartAsRoot
            accept();
            return;
        } else {
            m_errorLabel->setText(tr("Failed to restart with elevated privileges"));
            m_errorLabel->show();
        }
    } else {
        m_errorLabel->setText(tr("Authentication failed. Incorrect password."));
        m_errorLabel->show();
    }
    
    // Clear password from memory but leave field editable for retry
    password.fill('0');
    
    // Re-enable UI
    m_passwordEdit->setEnabled(true);
    m_authenticateButton->setEnabled(true);
    m_cancelButton->setEnabled(true);
    m_authenticateButton->setText(tr("Authenticate"));
    m_passwordEdit->setFocus();
    m_passwordEdit->selectAll();  // Select all text for easy replacement
}

bool ElevationDialog::elevateAndRestart(const QString &executablePath)
{
    // Use unified config file for language
    QString configPath = QDir::homePath() + "/.config/LinuxSystemViewer/LSV.conf";
    QByteArray envConfig = qgetenv("LSV_USER_CONFIG");
    if (!envConfig.isEmpty()) {
        configPath = QString::fromLocal8Bit(envConfig);
    }
    QSettings settings(configPath, QSettings::IniFormat);
    QString lang = settings.value("language", QString()).toString();
    QFile logFile("./logs/lsv_login_debug.log");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&logFile);
        ts << QDateTime::currentDateTime().toString(Qt::ISODate) << ": ElevationDialog configPath: " << configPath << ", lang: '" << lang << "'\n";
        logFile.close();
    }

    // Load translator for dialog
    QTranslator translator;
    if (!lang.isEmpty() && lang != "en") {
        QString qmPath = QString(":/i18n/lsv_%1.qm").arg(lang);
        if (!translator.load(qmPath)) {
            qWarning() << "Failed to load translation file:" << qmPath;
        }
        qApp->installTranslator(&translator);
    }

    ElevationDialog dialog;
    dialog.m_executablePath = executablePath;
    int result = dialog.exec();
    return result == QDialog::Accepted;
}

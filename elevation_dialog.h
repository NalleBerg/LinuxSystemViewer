#ifndef ELEVATION_DIALOG_H
#define ELEVATION_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QString>

/**
 * @brief Custom elevation dialog for requesting root privileges
 * 
 * This dialog prompts the user for their password to elevate the application
 * to root privileges. It uses PAM (Pluggable Authentication Modules) for 
 * secure authentication, providing a consistent experience across all Linux
 * distributions without depending on pkexec or terminal emulators.
 */
class ElevationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ElevationDialog(QWidget *parent = nullptr);
    ~ElevationDialog();

    /**
     * @brief Attempt to authenticate and restart the application with root privileges
     * @param executablePath Path to the executable to restart with elevated privileges
     * @return true if authentication succeeded and restart was initiated, false otherwise
     */
    static bool elevateAndRestart(const QString &executablePath);

private slots:
    void onAuthenticateClicked();

private:
    QLineEdit *m_passwordEdit;
    QLabel *m_messageLabel;
    QLabel *m_errorLabel;
    QPushButton *m_authenticateButton;
    QPushButton *m_cancelButton;
    QString m_executablePath;

    /**
     * @brief Authenticate using PAM with the provided password
     * @param username Username to authenticate as (current user)
     * @param password Password provided by user
     * @return true if authentication succeeded, false otherwise
     */
    bool authenticateWithPAM(const QString &username, const QString &password);

    /**
     * @brief Restart the application with root privileges using sudo
     * @param password The authenticated password
     * @return true if restart succeeded, false otherwise
     */
    bool restartAsRoot(const QString &password);
};

#endif // ELEVATION_DIALOG_H

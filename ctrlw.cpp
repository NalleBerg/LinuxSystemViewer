#include "ctrlw.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMainWindow>
#include <QDebug>
#include <QCloseEvent>
#include <QApplication>
#include <QCursor>
#include <QPainter>
#include <QElapsedTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

CtrlWHandler::CtrlWHandler(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    if (m_mainWindow) {
        // Install event filter on the main window to catch all key events
        m_mainWindow->installEventFilter(this);
        qDebug() << "CtrlWHandler: Event filter installed on main window";
        
        // Also install on the application to catch global events
        QApplication::instance()->installEventFilter(this);
        qDebug() << "CtrlWHandler: Global event filter installed";
    } else {
        qWarning() << "CtrlWHandler: Main window is null!";
    }
}

CtrlWHandler::~CtrlWHandler()
{
    qDebug() << "CtrlWHandler: Destructor called";
}

bool CtrlWHandler::eventFilter(QObject* obj, QEvent* event)
{
    // Debug all key events to see if we're catching them
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Debug output for all key presses
        qDebug() << "CtrlWHandler: Key pressed - Key:" << keyEvent->key() 
                 << "Modifiers:" << keyEvent->modifiers()
                 << "Text:" << keyEvent->text();
        
        // Check for Ctrl+W (multiple ways to be sure)
        bool isCtrlW = (keyEvent->modifiers() & Qt::ControlModifier) && 
                       (keyEvent->key() == Qt::Key_W);
        
        if (isCtrlW) {
            qDebug() << "CtrlWHandler: Ctrl+W detected! Showing quit dialog";
            // If the user confirms, close the main window just like the
            // close-event path does. Set a property so the close-event
            // handler doesn't prompt a second time.
            if (showQuitDialog()) {
                if (m_mainWindow) {
                    m_mainWindow->setProperty("confirmedQuit", true);
                    m_mainWindow->close();
                } else {
                    QApplication::quit();
                }
            }
            return true; // Event handled, don't pass it on
        }
        
        // Check for other quit shortcuts (Ctrl+Q, Alt+F4, etc.)
        if (keyEvent->matches(QKeySequence::Quit)) {
            qDebug() << "CtrlWHandler: Standard quit shortcut detected";
            if (showQuitDialog()) {
                if (m_mainWindow) {
                    m_mainWindow->setProperty("confirmedQuit", true);
                    m_mainWindow->close();
                } else {
                    QApplication::quit();
                }
            }
            return true;
        }
    }
    // Intercept window close events so we can show the confirmation dialog
    if (event->type() == QEvent::Close && obj == m_mainWindow) {
        QCloseEvent *ce = static_cast<QCloseEvent*>(event);
        // If a previous path already confirmed quit (e.g. Ctrl+W dialog),
        // allow the close to proceed without asking again.
        if (m_mainWindow && m_mainWindow->property("confirmedQuit").toBool()) {
            // clear the flag and allow close
            m_mainWindow->setProperty("confirmedQuit", false);
            return QObject::eventFilter(obj, event);
        }

        // If the main window is not the active window, the close likely
        // originated from a taskbar/context-menu action — in that case
        // perform a normal close without prompting.
        // If the cursor is outside the main window when the close happens
        // (for example the user right-clicked the panel/taskbar), treat
        // this as a taskbar/context close and skip confirmation. This helps
        // avoid prompting when the window manager invoked the close from a
        // panel action even if the app still appears active.
        QPoint globalCursor = QCursor::pos();
        if (m_mainWindow && !m_mainWindow->frameGeometry().contains(globalCursor)) {
            qDebug() << "CtrlWHandler: Close appears to come from outside window (cursor outside) — skipping confirmation";
            return QObject::eventFilter(obj, event);
        }

        // Fallback: if the main window is not the active window, likely a
        // taskbar/context-menu close — skip confirmation.
        if (!m_mainWindow->isActiveWindow() && QApplication::activeWindow() != m_mainWindow) {
            qDebug() << "CtrlWHandler: Close from taskbar/context — skipping confirmation";
            return QObject::eventFilter(obj, event);
        }

        // Otherwise show the quit confirmation dialog (clicking the X)
        qDebug() << "CtrlWHandler: Close event caught on main window — showing confirmation";
        ce->ignore(); // we'll close manually if the user confirms
        if (showQuitDialog()) {
            // User confirmed — close the window programmatically
            if (m_mainWindow) {
                m_mainWindow->setProperty("confirmedQuit", true);
                m_mainWindow->close();
            } else {
                QApplication::quit();
            }
        } else {
            qDebug() << "CtrlWHandler: User cancelled quit (close ignored)";
        }
        return true; // we handled the close
    }
    
    // Pass the event to the parent class
    return QObject::eventFilter(obj, event);
}

bool CtrlWHandler::showQuitDialog()
{
    qDebug() << "CtrlWHandler: Showing quit confirmation dialog";

    // Create a custom question icon: white '?' on a blue circular background
    const int ICON_SIZE = 48;
    QPixmap pix(ICON_SIZE, ICON_SIZE);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QColor blue(0x1E, 0x88, 0xE5); // pleasant blue
    painter.setBrush(blue);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, ICON_SIZE, ICON_SIZE);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSizeF(ICON_SIZE * 0.6);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pix.rect(), Qt::AlignCenter, QStringLiteral("?"));
    painter.end();

    // Build a minimal dialog: title only, icon centered above buttons
    QDialog dlg(m_mainWindow);
    dlg.setWindowTitle(QStringLiteral("Quit?"));
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QLabel *iconLabel = new QLabel(&dlg);
    iconLabel->setPixmap(pix);
    iconLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(iconLabel, 0, Qt::AlignHCenter);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, &dlg);
    QPushButton *yesBtn = box->button(QDialogButtonBox::Yes);
    QPushButton *noBtn = box->button(QDialogButtonBox::No);
    if (noBtn) noBtn->setDefault(true);
    lay->addWidget(box);

    QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    int res = dlg.exec();
    if (res == QDialog::Accepted) {
        qDebug() << "CtrlWHandler: User confirmed quit";
        return true;
    }
    qDebug() << "CtrlWHandler: User cancelled quit";
    return false;
}

void CtrlWHandler::showQuitDialogFromMenu()
{
    qDebug() << "CtrlWHandler: Quit requested from menu";
    if (showQuitDialog()) {
        if (m_mainWindow) {
            m_mainWindow->setProperty("confirmedQuit", true);
            m_mainWindow->close();
        } else {
            QApplication::quit();
        }
    }
}
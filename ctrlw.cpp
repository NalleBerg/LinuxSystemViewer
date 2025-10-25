#include "ctrlw.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMainWindow>
#include <QDebug>
#include <QCloseEvent>

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
            
            showQuitDialog();
            return true; // Event handled, don't pass it on
        }
        
        // Check for other quit shortcuts (Ctrl+Q, Alt+F4, etc.)
        if (keyEvent->matches(QKeySequence::Quit)) {
            qDebug() << "CtrlWHandler: Standard quit shortcut detected";
            showQuitDialog();
            return true;
        }
    }
    
    // Pass the event to the parent class
    return QObject::eventFilter(obj, event);
}

void CtrlWHandler::showQuitDialog()
{
    qDebug() << "CtrlWHandler: Showing quit confirmation dialog";
    
    int result = QMessageBox::question(
        m_mainWindow,
        "Quit LSV?",
        "Are you sure you want to quit Linux System Viewer?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes  // Default to Yes as requested
    );
    
    if (result == QMessageBox::Yes) {
        qDebug() << "CtrlWHandler: User confirmed quit";
        if (m_mainWindow) {
            // Set a flag to indicate this is a confirmed quit
            m_mainWindow->setProperty("confirmedQuit", true);
            m_mainWindow->close();
        } else {
            QApplication::quit();
        }
    } else {
        qDebug() << "CtrlWHandler: User cancelled quit";
    }
}

void CtrlWHandler::showQuitDialogFromMenu()
{
    qDebug() << "CtrlWHandler: Quit requested from menu";
    showQuitDialog();
}
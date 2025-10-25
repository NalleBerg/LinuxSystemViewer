#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSplitter>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QShowEvent>
#include <QDebug>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QCloseEvent>

// Version definition
#define VERSION "0.3.7"

// Forward declarations for main window class
class LinInfoGUI;

// Helper function declarations
void setIconsAggressively(QMainWindow* window);
void setupWindowProperties(QMainWindow* window);
void setupApplicationIcon(QApplication* app);
void applyApplicationStyleSheet(QMainWindow* window);
QIcon createApplicationIcon();

// Icon and styling helper functions
void setIconsAggressively(QMainWindow* window)
{
    qDebug() << "Setting LSV icons aggressively...";
    
    // Try multiple icon sources and sizes
    QIcon appIcon = createApplicationIcon();
    
    if (!appIcon.isNull()) {
        window->setWindowIcon(appIcon);
        qDebug() << "Icon loaded from embedded resource with multiple sizes";
    } else {
        qDebug() << "Failed to load icon from embedded resource";
    }
}

QIcon createApplicationIcon()
{
    QIcon appIcon;
    
    // Add multiple sizes from embedded resources
    appIcon.addFile(":/lsv.png", QSize(16, 16));
    appIcon.addFile(":/lsv.png", QSize(24, 24));
    appIcon.addFile(":/lsv.png", QSize(32, 32));
    appIcon.addFile(":/lsv.png", QSize(48, 48));
    appIcon.addFile(":/lsv.png", QSize(64, 64));
    appIcon.addFile(":/lsv.png", QSize(128, 128));
    
    // Add SVG for scalable icon
    appIcon.addFile(":/lsv.svg");
    
    return appIcon;
}

void setupWindowProperties(QMainWindow* window)
{
    window->setWindowTitle("Linux System Viewer (LSV) - V. " VERSION);
    
    // Set proper window size and properties
    window->setMinimumSize(600, 400);        // Minimum size
    window->resize(800, 600);                // Default size (800x600)
    
    // Ensure window has all standard window controls
    window->setWindowFlags(Qt::Window | 
                          Qt::WindowTitleHint | 
                          Qt::WindowSystemMenuHint | 
                          Qt::WindowMinimizeButtonHint | 
                          Qt::WindowMaximizeButtonHint | 
                          Qt::WindowCloseButtonHint);
    
    // Make sure window is resizable
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Set normal window state (not maximized, not fullscreen)
    window->setWindowState(Qt::WindowNoState);
}

void setupApplicationIcon(QApplication* app)
{
    QIcon appIcon = createApplicationIcon();
    app->setWindowIcon(appIcon);
    qDebug() << "Global application icon set successfully";
}

void applyApplicationStyleSheet(QMainWindow* window)
{
    // Style the application
    window->setStyleSheet(
        "QMainWindow { background-color: #f5f5f5; }"
        "QTabWidget::pane { border: 1px solid #c0c0c0; background-color: white; }"
        "QTabBar::tab { background-color: #e0e0e0; padding: 8px 16px; margin-right: 2px; border: 1px solid #c0c0c0; border-bottom: none; }"
        "QTabBar::tab:selected { background-color: white; border-bottom: 1px solid white; }"
        "QTabBar::tab:hover { background-color: #f0f0f0; }"
        "QTableWidget { gridline-color: #d0d0d0; background-color: white; }"
        "QTableWidget::item { padding: 8px; border-bottom: 1px solid #e0e0e0; }"
        "QTableWidget::item:selected { background-color: #3498db; color: white; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; }"
    );
}

// Table setup helper functions
QTableWidget* createStandardTable(const QStringList& headers)
{
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->setSortingEnabled(true);
    return table;
}

QTableWidget* createStorageTable(const QStringList& headers)
{
    QTableWidget* table = createStandardTable(headers);
    table->setSortingEnabled(false); // Disable sorting to maintain hierarchy
    return table;
}

// Application setup helper functions
void setupApplicationProperties(QApplication* app)
{
    app->setApplicationName("LSV");
    app->setApplicationVersion(VERSION);
    app->setOrganizationName("LSV");
    app->setOrganizationDomain("lsv.nalle.no");
}

// Icon refresh helper for show events
void refreshIconOnShow(QMainWindow* window)
{
    qDebug() << "Window shown, refreshing icon with multiple methods...";
    
    // Method 1: Re-set the window icon
    QIcon currentIcon = window->windowIcon();
    if (!currentIcon.isNull()) {
        window->setWindowIcon(currentIcon);
        qDebug() << "Window icon refreshed in showEvent with multiple methods";
    }
    
    // Method 2: Force window property update (X11 specific)
    QString desktopEnv = qgetenv("XDG_CURRENT_DESKTOP");
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    qDebug() << "Desktop environment:" << desktopEnv;
    qDebug() << "Session type:" << sessionType;
    
    // Method 3: Update window title to force refresh
    QString currentTitle = window->windowTitle();
    window->setWindowTitle(currentTitle);
    
    qDebug() << "Icon refresh completed for desktop environment:" << desktopEnv;
}

// Search functionality helpers
bool searchTabExists(QTabWidget* tabWidget, int& searchTabIndex)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->tabText(i) == "Search") {
            searchTabIndex = i;
            return true;
        }
    }
    return false;
}

int addSearchTab(QTabWidget* tabWidget, QTableWidget* searchTable)
{
    return tabWidget->addTab(searchTable, "Search");
}

void clearSearchResults(QTableWidget* searchTable)
{
    searchTable->setRowCount(0);
}

#endif // HELPER_FUNCTIONS_H
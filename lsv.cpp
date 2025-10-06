// Modular LinInfoGUI - Linux System Information Viewer
// Enhanced with modular headers and comprehensive physical disk information

#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QShowEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QWindow>
#include <QPixmap>

// Include our modular headers
#include "gui_helpers.h"
#include "storage.h"
#include "system_info.h" 
#include "memory.h"
#include "network.h"

#define VERSION "0.3.5"

class LinInfoGUI : public QMainWindow
{
    Q_OBJECT

public:
    LinInfoGUI(QWidget *parent = nullptr);
    ~LinInfoGUI();

private slots:
    void onRefreshClicked() { loadInitialData(); }
    
    void onSearchResultClicked(int row, int column);
    void performSearchAction();
    void closeSearchTab();
    void onTabCloseRequested(int index);
    void onTabChanged(int index);
    
    // Live refresh functions
    void refreshStorageTab();
    void refreshSummaryTab();
    void refreshMemoryTab();
    void refreshNetworkTab();
    void confirmQuit();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void loadInitialData();
    void updateSearchResults();
    void clearAllHighlighting();
    void highlightMatchedText(QTableWidget *table, int row, int col, const QString &searchTerm, bool useRegex = false);
    
    // UI Components
    QTabWidget *tabWidget;
    QTableWidget *summaryTable;
    QTableWidget *osTable;
    QTableWidget *systemTable;
    QTableWidget *cpuTable;
    QTableWidget *memoryTable;
    QTableWidget *storageTable;
    QTableWidget *networkTable;
    QTableWidget *searchTable;
    QPushButton *refreshButton;
    QLineEdit *searchField;
    QPushButton *searchButton;
    QCheckBox *regexCheckBox;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    
    // Data processing
    QTimer *refreshTimer;
    MemoryMonitor *memoryMonitor;
    QString searchTerm;
    int searchTabIndex; // Track search tab index for dynamic visibility
};

LinInfoGUI::LinInfoGUI(QWidget *parent)
    : QMainWindow(parent)
    , refreshTimer(new QTimer(this))
    , memoryMonitor(nullptr)
{
    setupUI();
    
    // Initialize memory monitor
    memoryMonitor = new MemoryMonitor(memoryTable, this);
    
    // Setup refresh timer for live updates (every 1 second)
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshStorageTab);
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshSummaryTab);
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshMemoryTab);
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshNetworkTab);
    refreshTimer->start(1000); // 1 second interval
    
    // Load initial data directly from system
    loadInitialData();
}

LinInfoGUI::~LinInfoGUI()
{
    // Clean destructor - timer cleanup handled automatically by Qt
}

void LinInfoGUI::setupUI()
{
    setWindowTitle(QString("🖥️ Linux System Viewer (LSV) - V. %1").arg(VERSION));
    setMinimumSize(800, 500);
    
    // Force window to show icon in title bar with multiple window flags
    setWindowFlags(windowFlags() | Qt::Window);
    
    // Set application icon for window and taskbar from embedded resource
    QIcon appIcon;
    appIcon.addFile(":lsv.png", QSize(16, 16));   // Small size for title bar
    appIcon.addFile(":lsv.png", QSize(24, 24));   // Small-medium size
    appIcon.addFile(":lsv.png", QSize(32, 32));   // Medium size for taskbar
    appIcon.addFile(":lsv.png", QSize(48, 48));   // Large size for alt-tab
    appIcon.addFile(":lsv.png", QSize(64, 64));   // Extra large
    appIcon.addFile(":lsv.svg");                  // SVG for scalability
    
    if (!appIcon.isNull()) {
        // Set icon for this specific window - try multiple times
        setWindowIcon(appIcon);
        setWindowIcon(appIcon);  // Set twice for stubborn WMs
        
        // Set icon for all application windows
        QApplication::setWindowIcon(appIcon);
        
        // Force window icon attribute
        setAttribute(Qt::WA_SetWindowIcon, true);
        
        // Try setting icon as pixmap too
        QPixmap iconPixmap = appIcon.pixmap(32, 32);
        if (!iconPixmap.isNull()) {
            setWindowIcon(QIcon(iconPixmap));
        }
        
        qDebug() << "Icon loaded from embedded resource with multiple sizes";
        qDebug() << "Available icon sizes:" << appIcon.availableSizes();
        qDebug() << "Window flags:" << windowFlags();
    } else {
        qDebug() << "Warning: Could not load embedded icon resource";
    }
    
    // Set compact font for the entire application
    QFont compactFont("Helvetica", 8);
    QApplication::setFont(compactFont);
    
    // Apply selective dark purple text color styling (not to table items)
    QString globalStyle = 
        "QLabel { color: #382a7e; }"
        "QPushButton { color: #382a7e; }"
        "QLineEdit { color: #382a7e; }"
        "QCheckBox { color: #382a7e; }"
        "QTabWidget::pane { color: #382a7e; }"
        "QTabBar::tab { color: #382a7e; }"
        "QMessageBox { color: #382a7e; }"
        "QMessageBox QLabel { color: #382a7e; }"
        "QMessageBox QPushButton { color: #382a7e; }"
        "QProgressBar { color: #382a7e; }"
        "QHeaderView::section { color: black; font-weight: bold; }";
    this->setStyleSheet(globalStyle);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    
    // Toolbar
    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setMaximumWidth(80);
    connect(refreshButton, &QPushButton::clicked, this, &LinInfoGUI::onRefreshClicked);
    // Initially hide refresh button since Summary tab (index 0) has auto-update
    refreshButton->setVisible(false);
    
    searchField = new QLineEdit(this);
    searchField->setPlaceholderText("Search system information...");
    searchField->setMaximumWidth(300);
    connect(searchField, &QLineEdit::returnPressed, this, &LinInfoGUI::performSearchAction);
    
    // Create search button
    searchButton = new QPushButton("Search", this);
    searchButton->setMaximumWidth(80);
    connect(searchButton, &QPushButton::clicked, this, &LinInfoGUI::performSearchAction);
    
    regexCheckBox = new QCheckBox("Regex", this);
    regexCheckBox->setToolTip("Enable regular expression search");
    
    statusLabel = new QLabel("Ready", this);
    
    progressBar = new QProgressBar(this);
    progressBar->setMaximumWidth(150);
    progressBar->hide();
    
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addWidget(searchField);
    toolbarLayout->addWidget(searchButton);
    toolbarLayout->addWidget(regexCheckBox);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(statusLabel);
    toolbarLayout->addWidget(progressBar);
    
    // Tab widget
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    connect(tabWidget, &QTabWidget::currentChanged, this, &LinInfoGUI::onTabChanged);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &LinInfoGUI::onTabCloseRequested);
    
    // Initialize all tables using our modular UI helpers
    summaryTable = new QTableWidget(this);
    initializeSummaryTable(summaryTable);
    
    osTable = new QTableWidget(this);
    initializeOSTable(osTable);
    
    systemTable = new QTableWidget(this);
    initializeSystemTable(systemTable);
    
    cpuTable = new QTableWidget(this);
    initializeCPUTable(cpuTable);
    
    memoryTable = new QTableWidget(this);
    initializeMemoryTable(memoryTable);
    
    storageTable = new QTableWidget(this);
    initializeStorageTable(storageTable);
    
    networkTable = new QTableWidget(this);
    initializeNetworkTable(networkTable);
    
    searchTable = new QTableWidget(this);
    initializeSearchTable(searchTable);
    connect(searchTable, &QTableWidget::cellClicked, this, &LinInfoGUI::onSearchResultClicked);
    searchTable->hide(); // Hide search table initially - it's only shown when added to tabs
    
    // Add tabs (search tab will be added dynamically when needed)
    tabWidget->addTab(summaryTable, "Summary");
    tabWidget->addTab(osTable, "OS");
    tabWidget->addTab(systemTable, "System");
    tabWidget->addTab(cpuTable, "CPU");
    tabWidget->addTab(memoryTable, "Memory");
    tabWidget->addTab(storageTable, "Storage");
    tabWidget->addTab(networkTable, "Network");
    
    // Initialize search tab index as -1 (not present)
    searchTabIndex = -1;
    
    // Make core tabs non-closeable
    for (int i = 0; i < tabWidget->count(); ++i) {
        tabWidget->tabBar()->setTabButton(i, QTabBar::RightSide, nullptr);
        tabWidget->tabBar()->setTabButton(i, QTabBar::LeftSide, nullptr);
    }
    
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(tabWidget);
}

void LinInfoGUI::loadInitialData()
{
    // Load all tabs with live system data (no lshw dependency)
    loadSummaryInformation(summaryTable);
    loadOSInformation(osTable, QJsonObject());           // Empty JSON, uses system calls
    loadSystemInformation(systemTable, QJsonObject());   // Empty JSON, uses system calls  
    loadCPUInformation(cpuTable, QJsonObject());         // Empty JSON, uses system calls
    loadMemoryInformation(memoryTable, QJsonObject());   // Empty JSON, uses system calls
    loadLiveStorageInformation(storageTable);            // Direct storage detection
    loadLiveNetworkInformation(networkTable);            // Direct network detection
    
    // Add live data to summary
    addLiveStorageToSummary(summaryTable);
    addLiveNetworkToSummary(summaryTable);
    
    statusLabel->setText("System information loaded");
}

void LinInfoGUI::performSearchAction()
{
    if (searchTerm.isEmpty()) {
        searchTerm = searchField->text();
    }
    
    if (searchTerm.length() < 2) {
        QMessageBox::information(this, "Search", "Please enter at least 2 characters to search.");
        return;
    }
    
    // Clear previous highlighting
    clearAllHighlighting();
    
    // Perform search using our modular search function
    QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
    QStringList tabNames = {"Summary", "OS", "System", "CPU", "Memory", "Storage", "Network"};
    
    bool useRegex = regexCheckBox->isChecked();
    QList<SearchResult> results = performSearch(searchTerm, tables, tabNames, useRegex);
    
    if (results.isEmpty()) {
        QMessageBox::information(this, "Search", "No results found for: " + searchTerm);
        return;
    }
    
    // Display results and show search tab
    displaySearchResults(searchTable, results);
    
    // Add search tab if not already present
    if (searchTabIndex == -1) {
        searchTabIndex = tabWidget->addTab(searchTable, "Search Results");
        // Make search tab closeable but others not
        QPushButton* closeButton = new QPushButton("×");
        closeButton->setFixedSize(20, 20);
        closeButton->setToolTip("Close search results");
        connect(closeButton, &QPushButton::clicked, this, &LinInfoGUI::closeSearchTab);
        tabWidget->tabBar()->setTabButton(searchTabIndex, QTabBar::RightSide, closeButton);
    }
    
    // Switch to search tab
    tabWidget->setCurrentIndex(searchTabIndex);
    
    // Highlight matches in all tables
    for (int i = 0; i < tables.size(); ++i) {
        QTableWidget* table = tables[i];
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                highlightMatchedText(table, row, col, searchTerm, useRegex);
            }
        }
    }
}

void LinInfoGUI::closeSearchTab()
{
    if (searchTabIndex != -1) {
        tabWidget->removeTab(searchTabIndex);
        searchTabIndex = -1;
        searchTable->setRowCount(0);
        clearAllHighlighting();
        searchField->clear();
        searchTerm.clear();
    }
}

void LinInfoGUI::onTabCloseRequested(int index)
{
    if (index == searchTabIndex) {
        closeSearchTab();
    }
}

void LinInfoGUI::onSearchResultClicked(int row, int column)
{
    Q_UNUSED(column);
    
    QTableWidgetItem* item = searchTable->item(row, 0);
    if (!item) return;
    
    SearchResult result = item->data(Qt::UserRole).value<SearchResult>();
    
    QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
    navigateToSearchResult(tabWidget, tables, result);
}

void LinInfoGUI::clearAllHighlighting()
{
    QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
    ::clearAllHighlighting(tables);
}

void LinInfoGUI::highlightMatchedText(QTableWidget *table, int row, int col, const QString &searchTerm, bool useRegex)
{
    ::highlightMatchedText(table, row, col, searchTerm, useRegex);
}

void LinInfoGUI::onTabChanged(int index)
{
    // Control refresh button visibility based on auto-update status
    // Summary (index 0), Memory (index 4), Storage (index 5), and Network (index 6) have auto-update
    bool hasAutoUpdate = (index == 0 || index == 4 || index == 5 || index == 6); // Summary, Memory, Storage, or Network
    refreshButton->setVisible(!hasAutoUpdate);
    
    // Force refresh of current tab's live data if needed
    refreshStorageTab();
    refreshSummaryTab();
    refreshMemoryTab();
    refreshNetworkTab();
}

void LinInfoGUI::refreshStorageTab()
{
    // Only refresh if the Storage tab is currently visible to save resources
    if (tabWidget->currentIndex() == 5) { // Storage tab is at index 5
        refreshStorageInfo(storageTable);
    }
}

void LinInfoGUI::refreshSummaryTab()
{
    // Only refresh if the Summary tab is currently visible to save resources
    if (tabWidget->currentIndex() == 0) { // Summary tab is at index 0
        loadSummaryInformation(summaryTable);
        addLiveStorageToSummary(summaryTable);
        addLiveNetworkToSummary(summaryTable);
    }
}

void LinInfoGUI::refreshMemoryTab()
{
    // Only refresh if the Memory tab is currently visible to save resources
    if (tabWidget->currentIndex() == 4) { // Memory tab is at index 4
        if (memoryMonitor && !memoryMonitor->isMonitoring()) {
            memoryMonitor->startMonitoring();
        }
    } else {
        // Stop monitoring when not on memory tab to save resources
        if (memoryMonitor && memoryMonitor->isMonitoring()) {
            memoryMonitor->stopMonitoring();
        }
    }
}

void LinInfoGUI::refreshNetworkTab()
{
    // Only refresh if the Network tab is currently visible to save resources
    if (tabWidget->currentIndex() == 6) { // Network tab is at index 6
        refreshNetworkInfo(networkTable);
    }
}

void LinInfoGUI::confirmQuit()
{
    // Create a custom dialog to avoid QMessageBox text issues
    QDialog dialog(this);
    dialog.setWindowTitle("Quit?");
    dialog.setModal(true);
    dialog.setFixedSize(200, 120);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Add question mark icon
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    
    // Add buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* yesButton = new QPushButton("Yes");
    QPushButton* noButton = new QPushButton("No");
    
    yesButton->setDefault(true);
    buttonLayout->addWidget(yesButton);
    buttonLayout->addWidget(noButton);
    layout->addLayout(buttonLayout);
    
    connect(yesButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(noButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Center dialog on parent
    dialog.move(this->geometry().center() - dialog.rect().center());
    
    if (dialog.exec() == QDialog::Accepted) {
        close();
    }
}

void LinInfoGUI::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_W) {
        confirmQuit();
        event->accept();
        return;
    }
    
    QMainWindow::keyPressEvent(event);
}

void LinInfoGUI::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    
    // Force refresh window icon after show event with multiple approaches
    QIcon appIcon;
    appIcon.addFile(":lsv.png", QSize(16, 16));
    appIcon.addFile(":lsv.png", QSize(24, 24));
    appIcon.addFile(":lsv.png", QSize(32, 32));
    appIcon.addFile(":lsv.svg");
    
    if (!appIcon.isNull()) {
        // Set icon using multiple methods
        setWindowIcon(appIcon);
        
        // Force window decoration update
        setAttribute(Qt::WA_SetWindowIcon, true);
        
        // Try to set icon on the native window handle
        if (windowHandle()) {
            windowHandle()->setIcon(appIcon);
        }
        
        // Force window update
        update();
        repaint();
        
        // Use QTimer to retry icon setting after window is fully shown
        QTimer::singleShot(100, this, [this, appIcon]() {
            setWindowIcon(appIcon);
            if (windowHandle()) {
                windowHandle()->setIcon(appIcon);
            }
            qDebug() << "Window icon re-applied with timer";
        });
        
        qDebug() << "Window icon refreshed in showEvent with multiple methods";
    }
}

void LinInfoGUI::closeEvent(QCloseEvent *event)
{
    // Create custom quit confirmation dialog
    QDialog* quitDialog = new QDialog(this);
    quitDialog->setWindowTitle("Quit?");
    quitDialog->setFixedSize(200, 120);
    quitDialog->setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(quitDialog);
    
    // Add question mark icon only (no text)
    QLabel* iconLabel = new QLabel();
    QPixmap icon = quitDialog->style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48);
    iconLabel->setPixmap(icon);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    
    // Add buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* yesButton = new QPushButton("Yes");
    QPushButton* noButton = new QPushButton("No");
    
    yesButton->setDefault(true);
    buttonLayout->addWidget(yesButton);
    buttonLayout->addWidget(noButton);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(yesButton, &QPushButton::clicked, quitDialog, &QDialog::accept);
    connect(noButton, &QPushButton::clicked, quitDialog, &QDialog::reject);
    
    // Show dialog and handle result
    int result = quitDialog->exec();
    
    if (result == QDialog::Accepted) {
        // Clean shutdown - stop timers
        if (refreshTimer) {
            refreshTimer->stop();
        }
        event->accept();
    } else {
        event->ignore();
    }
    
    quitDialog->deleteLater();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties for better system integration
    app.setApplicationName("LSV");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("NalleBerg");
    app.setOrganizationDomain("nalle.no");
    
    // Set application icon globally for better desktop integration
    QIcon appIcon;
    appIcon.addFile(":lsv.png", QSize(16, 16));
    appIcon.addFile(":lsv.png", QSize(24, 24));
    appIcon.addFile(":lsv.png", QSize(32, 32));
    appIcon.addFile(":lsv.png", QSize(48, 48));
    appIcon.addFile(":lsv.png", QSize(64, 64));
    appIcon.addFile(":lsv.svg");
    
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
        qDebug() << "Global application icon set successfully";
        qDebug() << "Desktop environment:" << qgetenv("XDG_CURRENT_DESKTOP");
        qDebug() << "Window manager:" << qgetenv("XDG_SESSION_TYPE");
        
        // Auto-configure desktop environment for optimal icon display
        QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
        if (desktop.contains("Cinnamon", Qt::CaseInsensitive)) {
            qDebug() << "Cinnamon detected - Window menu enabled for icon display";
        } else if (desktop.contains("GNOME", Qt::CaseInsensitive)) {
            qDebug() << "GNOME detected - Icon should appear in top bar and alt-tab";
        } else if (desktop.contains("KDE", Qt::CaseInsensitive) || desktop.contains("Plasma", Qt::CaseInsensitive)) {
            qDebug() << "KDE/Plasma detected - Icon should appear in title bar";
        } else if (desktop.contains("XFCE", Qt::CaseInsensitive)) {
            qDebug() << "XFCE detected - Icon should appear in title bar";
        } else if (desktop.contains("MATE", Qt::CaseInsensitive)) {
            qDebug() << "MATE detected - Icon should appear in title bar";
        } else {
            qDebug() << "Unknown desktop environment - Using default icon behavior";
        }
    } else {
        qDebug() << "Failed to load application icon";
    }
    
    LinInfoGUI window;
    window.show();
    
    return app.exec();
}

#include "lsv.moc"
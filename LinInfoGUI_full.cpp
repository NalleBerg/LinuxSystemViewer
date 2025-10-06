// Modular LinInfoGUI - Linux System Information Viewer
// Refactored into separate header modules for better maintainability

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QProgressBar>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QStorageInfo>
#include <QIcon>
#include <QFont>

// Include our modular headers
#include "ui_helpers.h"
#include "storage.h"
#include "system_info.h" 
#include "network.h"

#define VERSION "0.1.0"

class LinInfoGUI : public QMainWindow
{
    Q_OBJECT

public:
    LinInfoGUI(QWidget *parent = nullptr);
    ~LinInfoGUI();

private slots:
    void runLshw();
    void onLshwFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void runLshwFallback();
    void onLshwFallbackFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onLshwError(QProcess::ProcessError error);
    void onRefreshClicked() { runLshw(); }
    
    void onSearchResultClicked(int row, int column);
    void onSearchTextChanged(const QString &text);
    void onTabChanged(int index);
    
    // Live refresh functions
    void refreshStorageTab();
    void refreshSummaryTab();

private:
    void setupUI();
    void processLshwOutput(const QString &output);
    void processJsonData(const QJsonObject &obj, const QString &parentClass = "");
    void updateSearchResults();
    void clearAllHighlighting();
    void highlightMatchedText(QTableWidget *table, int row, int col, const QString &searchTerm);
    
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
    QLabel *statusLabel;
    QProgressBar *progressBar;
    
    // Data processing
    QProcess *lshwProcess;
    QTimer *refreshTimer;
    QString searchTerm;
    
    // System data storage
    QJsonObject systemData;
    QJsonObject cpuData;
    QJsonObject memoryData;
    QJsonArray networkData;
    QJsonArray storageData;
};

LinInfoGUI::LinInfoGUI(QWidget *parent)
    : QMainWindow(parent)
    , lshwProcess(nullptr)
    , refreshTimer(new QTimer(this))
{
    setupUI();
    
    // Setup refresh timer for live updates (every 1 second)
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshStorageTab);
    connect(refreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshSummaryTab);
    refreshTimer->start(1000); // 1 second interval
    
    // Initial data load
    runLshw();
}

LinInfoGUI::~LinInfoGUI()
{
    if (lshwProcess) {
        lshwProcess->kill();
        lshwProcess->waitForFinished(3000);
    }
}

void LinInfoGUI::setupUI()
{
    setWindowTitle(QString("Linux System Viewer - V. %1").arg(VERSION));
    setMinimumSize(800, 500);
    
    // Set application icon for window and taskbar from embedded resource
    QIcon appIcon(":LinInfoGUI.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
        QApplication::setWindowIcon(appIcon);
        qDebug() << "Icon loaded from embedded resource";
    } else {
        qDebug() << "Warning: Could not load embedded icon resource";
    }
    
    // Set compact font for the entire application
    QFont compactFont("Helvetica", 8);
    QApplication::setFont(compactFont);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    
    // Toolbar
    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setMaximumWidth(80);
    connect(refreshButton, &QPushButton::clicked, this, &LinInfoGUI::onRefreshClicked);
    
    searchField = new QLineEdit(this);
    searchField->setPlaceholderText("Search system information...");
    searchField->setMaximumWidth(300);
    connect(searchField, &QLineEdit::textChanged, this, &LinInfoGUI::onSearchTextChanged);
    
    statusLabel = new QLabel("Ready", this);
    
    progressBar = new QProgressBar(this);
    progressBar->setMaximumWidth(150);
    progressBar->hide();
    
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addWidget(new QLabel("Search:", this));
    toolbarLayout->addWidget(searchField);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(statusLabel);
    toolbarLayout->addWidget(progressBar);
    
    // Tab widget
    tabWidget = new QTabWidget(this);
    connect(tabWidget, &QTabWidget::currentChanged, this, &LinInfoGUI::onTabChanged);
    
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
    
    // Add tabs
    tabWidget->addTab(summaryTable, "Summary");
    tabWidget->addTab(osTable, "OS");
    tabWidget->addTab(systemTable, "System");
    tabWidget->addTab(cpuTable, "CPU");
    tabWidget->addTab(memoryTable, "Memory");
    tabWidget->addTab(storageTable, "Storage");
    tabWidget->addTab(networkTable, "Network");
    tabWidget->addTab(searchTable, "Search");
    
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(tabWidget);
}

void LinInfoGUI::runLshw()
{
    if (lshwProcess) {
        lshwProcess->kill();
        lshwProcess->waitForFinished(1000);
        delete lshwProcess;
    }
    
    lshwProcess = new QProcess(this);
    connect(lshwProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LinInfoGUI::onLshwFinished);
    connect(lshwProcess, &QProcess::errorOccurred, this, &LinInfoGUI::onLshwError);
    
    statusLabel->setText("Gathering system information...");
    progressBar->show();
    progressBar->setRange(0, 0);
    
    // Try lshw with sudo first
    lshwProcess->start("sudo", QStringList() << "lshw" << "-json");
    
    if (!lshwProcess->waitForStarted(3000)) {
        runLshwFallback();
    }
}

void LinInfoGUI::onLshwFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    progressBar->hide();
    
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = lshwProcess->readAllStandardOutput();
        processLshwOutput(output);
        statusLabel->setText("System information updated");
    } else {
        runLshwFallback();
    }
}

void LinInfoGUI::runLshwFallback()
{
    if (lshwProcess) {
        lshwProcess->kill();
        lshwProcess->waitForFinished(1000);
        delete lshwProcess;
    }
    
    lshwProcess = new QProcess(this);
    connect(lshwProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LinInfoGUI::onLshwFallbackFinished);
    
    statusLabel->setText("Trying fallback method...");
    lshwProcess->start("lshw", QStringList() << "-json");
}

void LinInfoGUI::onLshwFallbackFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    progressBar->hide();
    
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = lshwProcess->readAllStandardOutput();
        processLshwOutput(output);
        statusLabel->setText("System information updated (limited access)");
    } else {
        statusLabel->setText("Failed to gather system information");
        QMessageBox::warning(this, "Warning", "Could not run lshw. Some information may be unavailable.");
        
        // Load basic information using our modular functions
        loadSummaryInformation(summaryTable);
        loadOSInformation(osTable, QJsonObject());
        loadSystemInformation(systemTable, QJsonObject());
        loadCPUInformation(cpuTable, QJsonObject());
        loadMemoryInformation(memoryTable, QJsonObject());
        loadLiveStorageInformation(storageTable);
        loadLiveNetworkInformation(networkTable);
    }
}

void LinInfoGUI::onLshwError(QProcess::ProcessError error)
{
    progressBar->hide();
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start lshw. Please ensure it's installed.";
            break;
        case QProcess::Crashed:
            errorMsg = "lshw process crashed.";
            break;
        case QProcess::Timedout:
            errorMsg = "lshw process timed out.";
            break;
        default:
            errorMsg = "Unknown error occurred while running lshw.";
    }
    
    statusLabel->setText("Error: " + errorMsg);
    QMessageBox::critical(this, "Error", errorMsg);
}

void LinInfoGUI::processLshwOutput(const QString &output)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        statusLabel->setText("Error parsing system information");
        return;
    }
    
    QJsonObject rootObj = doc.object();
    processJsonData(rootObj);
    
    // Load information using our modular functions
    loadSummaryInformation(summaryTable);
    loadOSInformation(osTable, systemData);
    loadSystemInformation(systemTable, systemData);
    loadCPUInformation(cpuTable, cpuData);
    loadMemoryInformation(memoryTable, memoryData);
    loadLiveStorageInformation(storageTable);
    loadLiveNetworkInformation(networkTable);
    
    // Add live data to summary
    addLiveStorageToSummary(summaryTable);
    addLiveNetworkToSummary(summaryTable);
}

void LinInfoGUI::processJsonData(const QJsonObject &obj, const QString &parentClass)
{
    QString objClass = obj["class"].toString();
    
    // Store different types of data
    if (objClass == "system") {
        systemData = obj;
    } else if (objClass == "processor" || objClass == "cpu") {
        cpuData = obj;
    } else if (objClass == "memory") {
        memoryData = obj;
    } else if (objClass == "network") {
        networkData.append(obj);
    } else if (objClass == "disk" || objClass == "volume") {
        storageData.append(obj);
    }
    
    // Process children recursively
    if (obj.contains("children")) {
        QJsonArray children = obj["children"].toArray();
        for (const QJsonValue &child : children) {
            if (child.isObject()) {
                processJsonData(child.toObject(), objClass);
            }
        }
    }
}

void LinInfoGUI::onSearchTextChanged(const QString &text)
{
    searchTerm = text;
    updateSearchResults();
}

void LinInfoGUI::updateSearchResults()
{
    if (searchTerm.length() < 2) {
        searchTable->setRowCount(0);
        clearAllHighlighting();
        return;
    }
    
    // Clear previous highlighting
    clearAllHighlighting();
    
    // Perform search using our modular search function
    QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
    QStringList tabNames = {"Summary", "OS", "System", "CPU", "Memory", "Storage", "Network"};
    
    QList<SearchResult> results = performSearch(searchTerm, tables, tabNames);
    displaySearchResults(searchTable, results);
    
    // Highlight matches in all tables
    for (int i = 0; i < tables.size(); ++i) {
        QTableWidget* table = tables[i];
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                highlightMatchedText(table, row, col, searchTerm);
            }
        }
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

void LinInfoGUI::highlightMatchedText(QTableWidget *table, int row, int col, const QString &searchTerm)
{
    ::highlightMatchedText(table, row, col, searchTerm);
}

void LinInfoGUI::onTabChanged(int index)
{
    Q_UNUSED(index);
    // Force refresh of current tab's live data if needed
    refreshStorageTab();
    refreshSummaryTab();
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

// Required for QObject with Q_OBJECT macro
#include "LinInfoGUI.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties for better system integration
    app.setApplicationName("LinInfoGUI");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("NalleBerg");
    app.setOrganizationDomain("nalle.no");
    
    LinInfoGUI window;
    window.show();
    
    return app.exec();
}
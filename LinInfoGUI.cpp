#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QProgressBar>
#include <QMessageBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QTextStream>
#include <QStorageInfo>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QVariant>
#include <QColor>
#include <QBrush>
#include <QFont>

// Version constant for the application
const QString VERSION = "0.1.0";

// Search result navigation data
struct SearchResultData {
    int tabIndex;
    int rowIndex;
    int columnIndex;
    QString searchTerm;
};
Q_DECLARE_METATYPE(SearchResultData)

class LinInfoGUI : public QMainWindow
{
    Q_OBJECT

private:
    struct NetworkInfo {
        QString ipv4Address;
        QString ipv6Address;
        QString subnet;
        QString product;
        QString vendor;
        QString driver;
        bool isActive = false;
    };

public:
    LinInfoGUI(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setupUI();
        connectSignals();
        runLshw();
    }

private slots:
    void runLshw()
    {
        statusLabel->setText("Loading system information...");
        progressBar->setVisible(true);
        clearTables();
        loadSummaryInformation();
        loadOSInformation();
        
        QProcess *process = new QProcess(this);
        process->setProgram("lshw");
        process->setArguments({"-json"});
        
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &LinInfoGUI::onLshwFinished);
        connect(process, &QProcess::errorOccurred, this, &LinInfoGUI::onLshwError);
        
        process->start();
    }
    
    void onLshwFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        QProcess *process = qobject_cast<QProcess*>(sender());
        if (!process) return;
        
        progressBar->setVisible(false);
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QByteArray data = process->readAllStandardOutput();
            parseJsonData(data);
            statusLabel->setText("Hardware information loaded successfully");
        } else {
            runLshwFallback();
        }
        
        process->deleteLater();
    }
    
    void runLshwFallback()
    {
        statusLabel->setText("Trying alternative lshw format...");
        
        QProcess *process = new QProcess(this);
        process->setProgram("lshw");
        process->setArguments({"-short"});
        
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &LinInfoGUI::onLshwFallbackFinished);
        
        process->start();
    }
    
    void onLshwFallbackFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        QProcess *process = qobject_cast<QProcess*>(sender());
        if (!process) return;
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QByteArray data = process->readAllStandardOutput();
            parseShortFormat(data);
            statusLabel->setText("Hardware information loaded (short format)");
        } else {
            statusLabel->setText("Error: Could not run lshw. Make sure it's installed.");
            showErrorMessage();
        }
        
        process->deleteLater();
    }
    
    void onLshwError(QProcess::ProcessError error)
    {
        progressBar->setVisible(false);
        QString errorMsg;
        
        switch (error) {
            case QProcess::FailedToStart:
                errorMsg = "lshw failed to start. Make sure it's installed:\nsudo apt install lshw";
                break;
            case QProcess::Crashed:
                errorMsg = "lshw crashed during execution";
                break;
            default:
                errorMsg = "Unknown error occurred while running lshw";
                break;
        }
        
        statusLabel->setText("Error running lshw");
        QMessageBox::warning(this, "Error", errorMsg);
    }
    
    void onRefreshClicked() { runLshw(); }
    
    void onSearchResultClicked(int row, int column)
    {
        Q_UNUSED(column)
        
        // Get the navigation data from the first column (which stores it as user data)
        QTableWidgetItem *item = searchResultsTable->item(row, 0);
        if (!item) return;
        
        QVariant data = item->data(Qt::UserRole);
        if (!data.canConvert<SearchResultData>()) return;
        
        SearchResultData resultData = data.value<SearchResultData>();
        
        // Clear any previous highlighting
        clearAllHighlighting();
        
        // Navigate to the target tab
        tabWidget->setCurrentIndex(resultData.tabIndex);
        
        // Get the target table
        QTableWidget *targetTable = nullptr;
        QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
        if (resultData.tabIndex >= 0 && resultData.tabIndex < tables.size()) {
            targetTable = tables[resultData.tabIndex];
        }
        
        if (targetTable && resultData.rowIndex >= 0 && resultData.rowIndex < targetTable->rowCount()) {
            // Scroll to and select the row
            targetTable->selectRow(resultData.rowIndex);
            targetTable->scrollToItem(targetTable->item(resultData.rowIndex, 0));
            
            // Highlight the matched text
            highlightMatchedText(targetTable, resultData.rowIndex, resultData.columnIndex, resultData.searchTerm);
        }
    }
    
    void clearAllHighlighting()
    {
        // Clear highlighting from all tables
        QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
        for (QTableWidget *table : tables) {
            for (int row = 0; row < table->rowCount(); ++row) {
                for (int col = 0; col < table->columnCount(); ++col) {
                    QTableWidgetItem *item = table->item(row, col);
                    if (item) {
                        item->setBackground(QColor()); // Reset to default background
                    }
                }
            }
        }
    }
    
    void highlightMatchedText(QTableWidget *table, int row, int col, const QString &searchTerm)
    {
        QTableWidgetItem *item = table->item(row, col);
        if (item && item->text().contains(searchTerm, Qt::CaseInsensitive)) {
            // Highlight the matched cell with a yellow background
            item->setBackground(QColor(255, 255, 0, 100)); // Light yellow with transparency
        }
    }
    
    void onSearchTextChanged(const QString &text)
    {
        // Clear previous search results and highlighting
        searchResultsTable->setRowCount(0);
        clearAllHighlighting();
        
        if (text.isEmpty()) {
            // Hide search results tab and show all rows in all tabs
            tabWidget->setTabVisible(searchResultsTabIndex, false);
            
            for (int tabIndex = 0; tabIndex < tabWidget->count(); ++tabIndex) {
                if (tabIndex == searchResultsTabIndex) continue; // Skip search results tab
                
                QTableWidget *table = qobject_cast<QTableWidget*>(tabWidget->widget(tabIndex));
                if (!table) continue;
                
                for (int row = 0; row < table->rowCount(); ++row) {
                    table->setRowHidden(row, false);
                }
            }
            return;
        }
        
        // Collect search results from all tabs
        QStringList tabNames = {"Summary", "Operating System", "System", "CPU", "Memory", "Storage", "Network"};
        QList<QTableWidget*> tables = {summaryTable, osTable, systemTable, cpuTable, memoryTable, storageTable, networkTable};
        
        int totalResults = 0;
        
        for (int tabIndex = 0; tabIndex < tables.size(); ++tabIndex) {
            QTableWidget *table = tables[tabIndex];
            QString tabName = tabNames[tabIndex];
            
            for (int row = 0; row < table->rowCount(); ++row) {
                bool hasMatch = false;
                QString matchedProperty, matchedValue, matchedDetails;
                int matchedColumn = -1;
                
                // Check each column for matches
                for (int col = 0; col < table->columnCount(); ++col) {
                    QTableWidgetItem *item = table->item(row, col);
                    if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                        hasMatch = true;
                        if (matchedProperty.isEmpty()) {
                            matchedProperty = (col == 0) ? item->text() : 
                                            (table->horizontalHeaderItem(col) ? table->horizontalHeaderItem(col)->text() : QString("Column %1").arg(col));
                        }
                        if (matchedValue.isEmpty()) {
                            matchedValue = item->text();
                            matchedColumn = col;
                        }
                        // Collect additional details from the row
                        if (matchedDetails.isEmpty() && table->columnCount() > 1) {
                            QStringList rowData;
                            for (int detailCol = 0; detailCol < qMin(3, table->columnCount()); ++detailCol) {
                                QTableWidgetItem *detailItem = table->item(row, detailCol);
                                if (detailItem && !detailItem->text().isEmpty()) {
                                    rowData << detailItem->text();
                                }
                            }
                            matchedDetails = rowData.join(" | ");
                        }
                    }
                }
                
                if (hasMatch) {
                    // Create search result row with navigation data
                    int resultRow = searchResultsTable->rowCount();
                    searchResultsTable->insertRow(resultRow);
                    
                    // Create items for the result
                    QTableWidgetItem *tabItem = new QTableWidgetItem(tabName);
                    QTableWidgetItem *propertyItem = new QTableWidgetItem(matchedProperty);
                    QTableWidgetItem *valueItem = new QTableWidgetItem(matchedValue);
                    QTableWidgetItem *detailsItem = new QTableWidgetItem(matchedDetails);
                    
                    // Store navigation data in the first column
                    SearchResultData navData;
                    navData.tabIndex = tabIndex;
                    navData.rowIndex = row;
                    navData.columnIndex = matchedColumn;
                    navData.searchTerm = text;
                    tabItem->setData(Qt::UserRole, QVariant::fromValue(navData));
                    
                    // Make the row appear clickable
                    tabItem->setToolTip("Click to navigate to source");
                    propertyItem->setToolTip("Click to navigate to source");
                    valueItem->setToolTip("Click to navigate to source");
                    detailsItem->setToolTip("Click to navigate to source");
                    
                    // Set the items
                    searchResultsTable->setItem(resultRow, 0, tabItem);
                    searchResultsTable->setItem(resultRow, 1, propertyItem);
                    searchResultsTable->setItem(resultRow, 2, valueItem);
                    searchResultsTable->setItem(resultRow, 3, detailsItem);
                    
                    totalResults++;
                }
                
                // Hide/show rows in the original tab based on search
                table->setRowHidden(row, !hasMatch);
            }
        }
        
        // Show search results tab if we have results
        if (totalResults > 0) {
            tabWidget->setTabVisible(searchResultsTabIndex, true);
            // Update tab title with result count
            tabWidget->setTabText(searchResultsTabIndex, QString("Search Results (%1) - Click to Navigate").arg(totalResults));
        } else {
            tabWidget->setTabVisible(searchResultsTabIndex, false);
        }
    }
    
    void refreshNetworkTab()
    {
        // Only refresh if the Network tab is currently visible to save resources
        if (tabWidget->currentIndex() == 6) { // Network tab is at index 6
            // Refresh local network interfaces without external IP lookup
            refreshLocalNetworkInfo();
        }
    }
    
    void refreshLocalNetworkInfo()
    {
        // Get network interfaces and their status
        QMap<QString, NetworkInfo> networkInterfaces = getNetworkInterfaces();
        
        // Get default gateway
        QString defaultGateway = getDefaultGateway();
        
        // Update the table with current network information (keep existing external IPs)
        networkTable->setRowCount(0);
        populateNetworkTable(networkInterfaces, defaultGateway);
    }

private:
    void setupUI()
    {
        setWindowTitle(QString("Linux System Viewer, V %1").arg(VERSION));
        setMinimumSize(800, 500);  // Reduced minimum size
        
        // Set application icon for window and taskbar
        QIcon appIcon;
        
        // Try multiple icon paths for better compatibility
        QStringList iconPaths = {
            "LinInfoGUI.png",
            "./LinInfoGUI.png",
            QApplication::applicationDirPath() + "/LinInfoGUI.png",
            QApplication::applicationDirPath() + "/../LinInfoGUI.png"
        };
        
        bool iconLoaded = false;
        for (const QString &iconPath : iconPaths) {
            if (QFile::exists(iconPath)) {
                appIcon = QIcon(iconPath);
                if (!appIcon.isNull()) {
                    iconLoaded = true;
                    qDebug() << "Icon loaded from:" << iconPath;
                    break;
                }
            }
        }
        
        if (iconLoaded) {
            setWindowIcon(appIcon);
            QApplication::setWindowIcon(appIcon);
        } else {
            qDebug() << "Warning: Could not load LinInfoGUI.png icon from any location";
        }
        
        // Set compact font for the entire application
        QFont compactFont("Helvetica", 8);
        QApplication::setFont(compactFont);
        
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        QHBoxLayout *toolbarLayout = new QHBoxLayout();
        
        refreshButton = new QPushButton("Refresh", this);
        refreshButton->setMaximumWidth(80);
        refreshButton->setFont(QFont("Helvetica", 8));
        refreshButton->setMaximumHeight(24);
        
        QLabel *searchLabel = new QLabel("Search:", this);
        searchLabel->setFont(QFont("Helvetica", 8));
        searchEdit = new QLineEdit(this);
        searchEdit->setPlaceholderText("Type to search across all tabs...");
        searchEdit->setMaximumWidth(250);
        searchEdit->setFont(QFont("Helvetica", 8));
        searchEdit->setMaximumHeight(24);
        
        statusLabel = new QLabel("Ready", this);
        statusLabel->setFont(QFont("Helvetica", 8));
        progressBar = new QProgressBar(this);
        progressBar->setMaximumWidth(150);
        progressBar->setMaximumHeight(20);
        progressBar->setFont(QFont("Helvetica", 8));
        progressBar->setVisible(false);
        
        toolbarLayout->addWidget(refreshButton);
        toolbarLayout->addSpacing(20);
        toolbarLayout->addWidget(searchLabel);
        toolbarLayout->addWidget(searchEdit);
        toolbarLayout->addStretch();
        toolbarLayout->addWidget(statusLabel);
        toolbarLayout->addWidget(progressBar);
        
        mainLayout->addLayout(toolbarLayout);
        
        tabWidget = new QTabWidget(this);
        // Set compact styling for tabs
        tabWidget->setFont(QFont("Helvetica", 8));
        tabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #C0C0C0; }"
                                "QTabBar::tab { min-height: 18px; padding: 2px 8px; font-size: 8pt; }"
                                "QTabWidget { font-size: 8pt; }");
        mainLayout->addWidget(tabWidget);
        
        createHardwareTables();
    }
    
    void createHardwareTables()
    {
        summaryTable = createTable({"Property", "Value"});
        
        // Customize Summary table appearance
        summaryTable->verticalHeader()->setVisible(false); // Remove line numbers
        summaryTable->setStyleSheet(
            "QTableWidget { font-size: 8pt; gridline-color: #E0E0E0; }"
            "QHeaderView::section { font-size: 8pt; font-weight: bold; padding: 2px; }"
            "QTableWidget::item:nth-child(1) { font-weight: bold; }" // Property column bold
        );
        
        tabWidget->addTab(summaryTable, "Summary");
        
        osTable = createTable({"Property", "Value"});
        tabWidget->addTab(osTable, "Operating System");
        
        systemTable = createTable({"Property", "Value"});
        tabWidget->addTab(systemTable, "System");
        
        cpuTable = createTable({"Property", "Value", "Details"});
        tabWidget->addTab(cpuTable, "CPU");
        
        memoryTable = createTable({"Bank", "Size", "Type", "Speed", "Description"});
        tabWidget->addTab(memoryTable, "Memory");
        
        storageTable = createTable({"Device", "Size", "Type", "Model", "Description"});
        tabWidget->addTab(storageTable, "Storage");
        
        networkTable = createTable({"Interface", "Status", "IPv4 Address", "IPv6 Address", "Subnet", "Gateway", "Product", "Vendor", "Driver"});
        tabWidget->addTab(networkTable, "Network");

        // Create search results tab (initially hidden)
        searchResultsTable = createTable({"Found In Tab", "Property", "Value", "Details"});
        searchResultsTabIndex = tabWidget->addTab(searchResultsTable, "Search Results");
        tabWidget->setTabVisible(searchResultsTabIndex, false);
    }
    
    QTableWidget* createTable(const QStringList &headers)
    {
        QTableWidget *table = new QTableWidget(this);
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        
        // Apply compact styling
        table->setFont(QFont("Helvetica", 8));
        table->verticalHeader()->setDefaultSectionSize(40);  // Increased row height for multi-line content
        table->horizontalHeader()->setFont(QFont("Helvetica", 8, QFont::Bold));
        table->horizontalHeader()->setDefaultSectionSize(80); // Compact column width
        table->horizontalHeader()->setStretchLastSection(true);
        
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSortingEnabled(true);
        table->setWordWrap(true);  // Enable word wrapping for multi-line content
        
        // Compact table styling
        table->setStyleSheet("QTableWidget { font-size: 8pt; gridline-color: #E0E0E0; }"
                            "QHeaderView::section { font-size: 8pt; font-weight: bold; padding: 2px; }");
        
        return table;
    }
    
    void connectSignals()
    {
        connect(refreshButton, &QPushButton::clicked, this, &LinInfoGUI::onRefreshClicked);
        connect(searchEdit, &QLineEdit::textChanged, this, &LinInfoGUI::onSearchTextChanged);
        connect(searchResultsTable, &QTableWidget::cellClicked, this, &LinInfoGUI::onSearchResultClicked);
        
        // Setup network auto-refresh timer
        networkRefreshTimer = new QTimer(this);
        connect(networkRefreshTimer, &QTimer::timeout, this, &LinInfoGUI::refreshNetworkTab);
        networkRefreshTimer->start(1000); // Refresh every second
    }
    
    void clearTables()
    {
        summaryTable->setRowCount(0);
        osTable->setRowCount(0);
        systemTable->setRowCount(0);
        cpuTable->setRowCount(0);
        memoryTable->setRowCount(0);
        storageTable->setRowCount(0);
        networkTable->setRowCount(0);
        searchResultsTable->setRowCount(0);
    }
    
    void parseJsonData(const QByteArray &data)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << error.errorString();
            parseShortFormat(data);
            return;
        }
        
        QJsonArray items;
        if (doc.isArray()) {
            items = doc.array();
        } else if (doc.isObject()) {
            items.append(doc.object());
        }
        
        processJsonItems(items);
    }
    
    void processJsonItems(const QJsonArray &items)
    {
        for (const QJsonValue &value : items) {
            if (!value.isObject()) continue;
            
            QJsonObject item = value.toObject();
            processJsonItem(item);
            
            if (item.contains("children")) {
                QJsonArray children = item["children"].toArray();
                processJsonItems(children);
            }
        }
    }
    
    void processJsonItem(const QJsonObject &item)
    {
        QString id = item["id"].toString();
        QString className = item["class"].toString();
        QString product = item["product"].toString();
        QString vendor = item["vendor"].toString();
        QString description = item["description"].toString();
        
        // Process hardware information by category
        if (className == "system") {
            loadSystemInformation(item);
        } else if (className == "processor" || className == "cpu") {
            loadCpuInformation(item);
        } else if (className == "memory" || className == "bank") {
            loadMemoryInformation(item);
        } else if (className == "disk" || className == "storage") {
            loadStorageInformation(item);
        } else if (className == "network") {
            loadNetworkInformation(item);
        }
    }
    
    void loadSystemInformation(const QJsonObject &item)
    {
        addPropertyToTable(systemTable, "Product", item["product"].toString());
        addPropertyToTable(systemTable, "Vendor", item["vendor"].toString());
        addPropertyToTable(systemTable, "Version", item["version"].toString());
        addPropertyToTable(systemTable, "Serial", item["serial"].toString());
        
        if (item.contains("configuration")) {
            QJsonObject config = item["configuration"].toObject();
            for (auto it = config.begin(); it != config.end(); ++it) {
                addPropertyToTable(systemTable, "Config: " + it.key(), it.value().toString());
            }
        }
    }
    
    void loadCpuInformation(const QJsonObject &item)
    {
        QString details = QString("Cores: %1, Threads: %2")
                         .arg(item["configuration"].toObject()["cores"].toString())
                         .arg(item["configuration"].toObject()["threads"].toString());
        
        addRowToTable(cpuTable, {"Product", item["product"].toString(), details});
        addRowToTable(cpuTable, {"Vendor", item["vendor"].toString(), item["description"].toString()});
        
        if (item.contains("size")) {
            addRowToTable(cpuTable, {"Current Speed", 
                QString::number(item["size"].toDouble() / 1000000) + " MHz", ""});
        }
    }
    
    void loadMemoryInformation(const QJsonObject &item)
    {
        QString size = formatSize(item["size"].toDouble());
        QString description = item["description"].toString();
        
        addRowToTable(memoryTable, {
            item["slot"].toString(),
            size,
            item["product"].toString(),
            item["clock"].toString() + " MHz",
            description
        });
    }
    
    void loadStorageInformation(const QJsonObject &item)
    {
        QString size = formatSize(item["size"].toDouble());
        QString type = item["description"].toString();
        
        addRowToTable(storageTable, {
            item["logicalname"].toString(),
            size,
            type,
            item["product"].toString(),
            item["vendor"].toString()
        });
    }
    
    QMap<QString, NetworkInfo> getNetworkInterfaces()
    {
        QMap<QString, NetworkInfo> interfaces;
        
        // Get all network interfaces from /sys/class/net
        QDir netDir("/sys/class/net");
        QStringList interfaceNames = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QString &interfaceName : interfaceNames) {
            if (interfaceName == "lo") continue; // Skip loopback
            
            NetworkInfo info;
            
            // Check if interface is up
            QFile operState(QString("/sys/class/net/%1/operstate").arg(interfaceName));
            if (operState.open(QIODevice::ReadOnly)) {
                QString state = operState.readAll().trimmed();
                info.isActive = (state == "up");
            }
            
            // Get IP addresses using ip command
            QProcess ipProcess;
            ipProcess.start("ip", QStringList() << "addr" << "show" << interfaceName);
            ipProcess.waitForFinished(2000);
            
            if (ipProcess.exitCode() == 0) {
                QString output = ipProcess.readAllStandardOutput();
                
                // IPv4 pattern
                QRegularExpression ipv4Regex(R"(inet (\d+\.\d+\.\d+\.\d+)/(\d+))");
                QRegularExpressionMatch ipv4Match = ipv4Regex.match(output);
                
                if (ipv4Match.hasMatch()) {
                    info.ipv4Address = ipv4Match.captured(1);
                    int prefixLength = ipv4Match.captured(2).toInt();
                    info.subnet = prefixLengthToSubnetMask(prefixLength);
                }
                
                // IPv6 pattern (global unicast addresses)
                QRegularExpression ipv6Regex(R"(inet6 ([0-9a-fA-F:]+)/(\d+) scope global)");
                QRegularExpressionMatch ipv6Match = ipv6Regex.match(output);
                
                if (ipv6Match.hasMatch()) {
                    info.ipv6Address = ipv6Match.captured(1);
                }
            }
            
            // Get hardware info using ethtool
            QProcess ethtoolProcess;
            ethtoolProcess.start("ethtool", QStringList() << "-i" << interfaceName);
            ethtoolProcess.waitForFinished(2000);
            
            if (ethtoolProcess.exitCode() == 0) {
                QString output = ethtoolProcess.readAllStandardOutput();
                QStringList lines = output.split('\n');
                
                for (const QString &line : lines) {
                    if (line.startsWith("driver: ")) {
                        info.driver = line.mid(8).trimmed();
                    }
                }
            }
            
            interfaces[interfaceName] = info;
        }
        
        return interfaces;
    }
    
    QString prefixLengthToSubnetMask(int prefixLength)
    {
        if (prefixLength < 0 || prefixLength > 32) return "Invalid";
        
        uint32_t mask = (0xFFFFFFFF << (32 - prefixLength)) & 0xFFFFFFFF;
        return QString("%1.%2.%3.%4")
            .arg((mask >> 24) & 0xFF)
            .arg((mask >> 16) & 0xFF)
            .arg((mask >> 8) & 0xFF)
            .arg(mask & 0xFF);
    }
    
    QString getDefaultGateway()
    {
        QProcess routeProcess;
        routeProcess.start("ip", QStringList() << "route" << "show" << "default");
        routeProcess.waitForFinished(2000);
        
        if (routeProcess.exitCode() == 0) {
            QString output = routeProcess.readAllStandardOutput();
            QRegularExpression gwRegex(R"(default via (\d+\.\d+\.\d+\.\d+))");
            QRegularExpressionMatch match = gwRegex.match(output);
            
            if (match.hasMatch()) {
                return match.captured(1);
            }
        }
        
        return QString();
    }
    
    void getExternalIP()
    {
        qDebug() << "Starting external IP lookup...";
        
        // Get IPv4 external IP
        QProcess *curl4Process = new QProcess(this);
        connect(curl4Process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, curl4Process](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        QString ipv4 = curl4Process->readAllStandardOutput().trimmed();
                        if (!ipv4.isEmpty()) {
                            externalIPv4 = ipv4;
                            qDebug() << "Got external IPv4:" << externalIPv4;
                        }
                    }
                    curl4Process->deleteLater();
                    
                    // Now get IPv6 external IP
                    getExternalIPv6();
                });
        
        curl4Process->start("curl", QStringList() << "-4" << "-s" << "--max-time" << "5" << "ifconfig.me");
    }
    
    void getExternalIPv6()
    {
        QProcess *curl6Process = new QProcess(this);
        connect(curl6Process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, curl6Process](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        QString ipv6 = curl6Process->readAllStandardOutput().trimmed();
                        if (!ipv6.isEmpty()) {
                            externalIPv6 = ipv6;
                            qDebug() << "Got external IPv6:" << externalIPv6;
                        }
                    }
                    
                    curl6Process->deleteLater();
                    
                    // Reset the external IP lookup flag
                    setExternalIPLookupFlag(false);
                    
                    qDebug() << "Final refresh with external IPs:" << externalIPv4 << externalIPv6;
                    // Only refresh the table display, don't trigger another lookup
                    updateNetworkTable();
                });
        
        curl6Process->start("curl", QStringList() << "-6" << "-s" << "--max-time" << "5" << "ifconfig.me");
    }
    
    void setExternalIPLookupFlag(bool value)
    {
        static bool isLookingUpExternalIP = false;
        isLookingUpExternalIP = value;
    }
    
    bool getExternalIPLookupFlag()
    {
        static bool isLookingUpExternalIP = false;
        return isLookingUpExternalIP;
    }
    
    void loadNetworkInformationFromSystem()
    {
        networkTable->setRowCount(0);
        
        // Get network interfaces and their status
        QMap<QString, NetworkInfo> networkInterfaces = getNetworkInterfaces();
        
        // Get default gateway
        QString defaultGateway = getDefaultGateway();
        
        // Start external IP lookup only if not already in progress
        if (!getExternalIPLookupFlag()) {
            setExternalIPLookupFlag(true);
            getExternalIP();
        }
        
        // Populate the table with current network information
        populateNetworkTable(networkInterfaces, defaultGateway);
    }
    
    void updateNetworkTable()
    {
        qDebug() << "updateNetworkTable() called with external IPs:" << externalIPv4 << externalIPv6;
        // Get fresh network data without triggering external IP lookup
        QMap<QString, NetworkInfo> networkInterfaces = getNetworkInterfaces();
        QString defaultGateway = getDefaultGateway();
        
        networkTable->setRowCount(0);
        populateNetworkTable(networkInterfaces, defaultGateway);
    }
    
    void populateNetworkTable(const QMap<QString, NetworkInfo> &networkInterfaces, const QString &defaultGateway)
    {
        int row = 0;
        for (auto it = networkInterfaces.begin(); it != networkInterfaces.end(); ++it) {
            const QString &interface = it.key();
            const NetworkInfo &info = it.value();
            
            networkTable->insertRow(row);
            
            // Interface name
            QTableWidgetItem *interfaceItem = new QTableWidgetItem(interface);
            if (!info.isActive) {
                interfaceItem->setForeground(QColor(128, 128, 128)); // Grey out inactive
                interfaceItem->setFont(QFont("Helvetica", 8, QFont::Normal, true)); // Italic
            }
            networkTable->setItem(row, 0, interfaceItem);
            
            // Status
            QTableWidgetItem *statusItem = new QTableWidgetItem(info.isActive ? "Active" : "Inactive");
            statusItem->setForeground(info.isActive ? QColor(0, 128, 0) : QColor(128, 128, 128));
            if (!info.isActive) {
                statusItem->setFont(QFont("Helvetica", 8, QFont::Normal, true)); // Italic
            }
            networkTable->setItem(row, 1, statusItem);
            
            // IPv4 Address
            QString ipv4Text = info.ipv4Address.isEmpty() ? "N/A" : info.ipv4Address;
            QTableWidgetItem *ipv4Item = createColoredIPItem(ipv4Text, false, info.isActive && !info.ipv4Address.isEmpty(), true);
            ipv4Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            networkTable->setItem(row, 2, ipv4Item);
            
            // IPv6 Address
            QString ipv6Text = info.ipv6Address.isEmpty() ? "N/A" : info.ipv6Address;
            QTableWidgetItem *ipv6Item = createColoredIPItem(ipv6Text, true, info.isActive && !info.ipv6Address.isEmpty(), true);
            ipv6Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            networkTable->setItem(row, 3, ipv6Item);
            
            // Subnet
            QTableWidgetItem *subnetItem = new QTableWidgetItem(info.subnet.isEmpty() ? "N/A" : info.subnet);
            subnetItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (!info.isActive) {
                subnetItem->setForeground(QColor(128, 128, 128));
                subnetItem->setFont(QFont("Helvetica", 8, QFont::Normal, true));
            }
            networkTable->setItem(row, 4, subnetItem);
            
            // Gateway (show default gateway for active interfaces)
            QString gatewayText = info.isActive && !defaultGateway.isEmpty() ? defaultGateway : "N/A";
            QTableWidgetItem *gatewayItem = new QTableWidgetItem(gatewayText);
            gatewayItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (!info.isActive) {
                gatewayItem->setForeground(QColor(128, 128, 128));
                gatewayItem->setFont(QFont("Helvetica", 8, QFont::Normal, true));
            }
            networkTable->setItem(row, 5, gatewayItem);
            
            // Product (will be filled by lshw data)
            QTableWidgetItem *productItem = new QTableWidgetItem(info.product.isEmpty() ? "Unknown" : info.product);
            if (!info.isActive) {
                productItem->setForeground(QColor(128, 128, 128));
                productItem->setFont(QFont("Helvetica", 8, QFont::Normal, true));
            }
            networkTable->setItem(row, 6, productItem);
            
            // Vendor (will be filled by lshw data)
            QTableWidgetItem *vendorItem = new QTableWidgetItem(info.vendor.isEmpty() ? "Unknown" : info.vendor);
            if (!info.isActive) {
                vendorItem->setForeground(QColor(128, 128, 128));
                vendorItem->setFont(QFont("Helvetica", 8, QFont::Normal, true));
            }
            networkTable->setItem(row, 7, vendorItem);
            
            // Driver
            QTableWidgetItem *driverItem = new QTableWidgetItem(info.driver.isEmpty() ? "Unknown" : info.driver);
            if (!info.isActive) {
                driverItem->setForeground(QColor(128, 128, 128));
                driverItem->setFont(QFont("Helvetica", 8, QFont::Normal, true));
            }
            networkTable->setItem(row, 8, driverItem);
            
            row++;
        }
        
        // Add a dedicated External IP row if we have external IPs
        if (!externalIPv4.isEmpty() || !externalIPv6.isEmpty()) {
            // Add an empty separator row first
            networkTable->insertRow(row);
            for (int col = 0; col < 9; col++) {
                networkTable->setItem(row, col, new QTableWidgetItem(""));
            }
            row++;
            
            // Now add the External IP row
            networkTable->insertRow(row);
            
            // Interface column - "External IP:"
            QTableWidgetItem *extLabelItem = new QTableWidgetItem("External IP:");
            extLabelItem->setFont(QFont("Helvetica", 8, QFont::Bold));
            extLabelItem->setForeground(QColor(0, 0, 0)); // Black
            networkTable->setItem(row, 0, extLabelItem);
            
            // Status column - empty
            networkTable->setItem(row, 1, new QTableWidgetItem(""));
            
            // IPv4 column - external IPv4
            QString extIPv4Text = externalIPv4.isEmpty() ? "N/A" : externalIPv4;
            QTableWidgetItem *extIPv4Item = new QTableWidgetItem(extIPv4Text);
            extIPv4Item->setFont(QFont("Helvetica", 8, QFont::Bold));
            extIPv4Item->setForeground(QColor(0, 128, 0)); // Green for IPv4
            extIPv4Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            networkTable->setItem(row, 2, extIPv4Item);
            
            // IPv6 column - external IPv6
            QString extIPv6Text = externalIPv6.isEmpty() ? "N/A" : externalIPv6;
            QTableWidgetItem *extIPv6Item = new QTableWidgetItem(extIPv6Text);
            extIPv6Item->setFont(QFont("Helvetica", 8, QFont::Bold));
            extIPv6Item->setForeground(QColor(0, 0, 139)); // Dark blue for IPv6
            extIPv6Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            networkTable->setItem(row, 3, extIPv6Item);
            
            // Fill remaining columns with empty cells
            for (int col = 4; col < 9; col++) {
                networkTable->setItem(row, col, new QTableWidgetItem(""));
            }
        }
        
        networkTable->resizeColumnsToContents();
        networkTable->resizeRowsToContents();  // Make sure rows are tall enough for multi-line content
        
        // Ensure IP address columns have enough width to prevent ellipsis
        networkTable->setColumnWidth(2, qMax(networkTable->columnWidth(2), 200)); // IPv4 column
        networkTable->setColumnWidth(3, qMax(networkTable->columnWidth(3), 200)); // IPv6 column
    }
    
    QTableWidgetItem* createColoredIPItem(const QString &ip, bool isIPv6, bool isActive, bool isBold = false)
    {
        QTableWidgetItem *item = new QTableWidgetItem(ip);
        
        if (!isActive || ip == "N/A") {
            item->setForeground(QColor(128, 128, 128));
            item->setFont(QFont("Helvetica", 8, QFont::Normal, true));
        } else {
            if (isIPv6) {
                item->setForeground(QColor(0, 0, 139)); // Dark blue for IPv6
            } else {
                item->setForeground(QColor(0, 128, 0)); // Green for IPv4
            }
            if (isBold) {
                item->setFont(QFont("Helvetica", 8, QFont::Bold));
            } else {
                item->setFont(QFont("Helvetica", 8, QFont::Normal));
            }
        }
        
        return item;
    }

    void loadNetworkInformation(const QJsonObject &item)
    {
        // Always load system network information to ensure fresh data
        loadNetworkInformationFromSystem();
        
        // Update hardware info from lshw data
        QString logicalName = item["logicalname"].toString();
        QString product = item["product"].toString();
        QString vendor = item["vendor"].toString();
        
        // Find existing row with this interface and update it
        for (int row = 0; row < networkTable->rowCount(); row++) {
            QTableWidgetItem *interfaceItem = networkTable->item(row, 0);
            if (interfaceItem && interfaceItem->text() == logicalName) {
                // Update product
                if (!product.isEmpty() && networkTable->item(row, 7)) {
                    networkTable->item(row, 7)->setText(product);
                }
                // Update vendor
                if (!vendor.isEmpty() && networkTable->item(row, 8)) {
                    networkTable->item(row, 8)->setText(vendor);
                }
                break;
            }
        }
    }
    
    void parseShortFormat(const QByteArray &data)
    {
        QString text = QString::fromUtf8(data);
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        
        for (const QString &line : lines) {
            if (line.startsWith("H/W path")) continue;
            
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString device = parts[0];
                QString className = parts[1];
                QString description = parts.mid(2).join(" ");
                
                // Note: Short format parsing - detailed categorization not available
            }
        }
        
        statusLabel->setText("Hardware information loaded (simplified format)");
    }
    
    void addRowToTable(QTableWidget *table, const QStringList &values)
    {
        int row = table->rowCount();
        table->insertRow(row);
        
        for (int i = 0; i < values.size() && i < table->columnCount(); ++i) {
            table->setItem(row, i, new QTableWidgetItem(values[i]));
        }
    }
    
    void showErrorMessage()
    {
        QMessageBox::information(this, "Install lshw", 
            "To use this application, please install lshw:\n\n"
            "sudo apt install lshw\n\n"
            "After installation, click Refresh to load hardware information.");
    }
    
    void loadOSInformation()
    {
        addPropertyToTable(osTable, "Operating System", QSysInfo::prettyProductName());
        addPropertyToTable(osTable, "Kernel Type", QSysInfo::kernelType());
        addPropertyToTable(osTable, "Kernel Version", QSysInfo::kernelVersion());
        addPropertyToTable(osTable, "Architecture", QSysInfo::currentCpuArchitecture());
        addPropertyToTable(osTable, "Build ABI", QSysInfo::buildAbi());
        addPropertyToTable(osTable, "Hostname", QSysInfo::machineHostName());
        
        QString user = qgetenv("USER");
        if (user.isEmpty()) user = qgetenv("USERNAME");
        addPropertyToTable(osTable, "Current User", user);
        
        QString shell = qgetenv("SHELL");
        addPropertyToTable(osTable, "Default Shell", shell);
        
        QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
        if (desktop.isEmpty()) desktop = qgetenv("DESKTOP_SESSION");
        if (!desktop.isEmpty()) {
            addPropertyToTable(osTable, "Desktop Environment", desktop);
        }
        
        QFile versionFile("/proc/version");
        if (versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString version = versionFile.readAll().trimmed();
            addPropertyToTable(osTable, "Kernel Details", version);
        }
        
        QFile uptimeFile("/proc/uptime");
        if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString uptimeStr = uptimeFile.readAll().split(' ')[0];
            double uptimeSeconds = uptimeStr.toDouble();
            int days = uptimeSeconds / 86400;
            int hours = (int(uptimeSeconds) % 86400) / 3600;
            int minutes = (int(uptimeSeconds) % 3600) / 60;
            
            QString formattedUptime = QString("%1 days, %2 hours, %3 minutes")
                                     .arg(days).arg(hours).arg(minutes);
            addPropertyToTable(osTable, "System Uptime", formattedUptime);
        }
        
        QFile releaseFile("/etc/os-release");
        if (releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&releaseFile);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.contains('=')) {
                    QStringList parts = line.split('=', Qt::SkipEmptyParts);
                    if (parts.size() >= 2) {
                        QString key = parts[0];
                        QString value = parts[1].remove('"');
                        
                        if (key == "NAME") {
                            addPropertyToTable(osTable, "Distribution", value);
                        } else if (key == "VERSION") {
                            addPropertyToTable(osTable, "Distribution Version", value);
                        } else if (key == "PRETTY_NAME") {
                            addPropertyToTable(osTable, "Full Name", value);
                        }
                    }
                }
            }
        }
        
        QFile memFile("/proc/meminfo");
        if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&memFile);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.startsWith("MemTotal:")) {
                    QString memStr = line.split(QRegularExpression("\\s+"))[1];
                    double memKB = memStr.toDouble();
                    QString memFormatted = formatSize(memKB * 1024);
                    addPropertyToTable(osTable, "Total Memory", memFormatted);
                    break;
                }
            }
        }
        
        QStorageInfo storage = QStorageInfo::root();
        if (storage.isValid()) {
            QString totalSpace = formatSize(storage.bytesTotal());
            QString freeSpace = formatSize(storage.bytesAvailable());
            addPropertyToTable(osTable, "Root Filesystem", storage.rootPath());
            addPropertyToTable(osTable, "Total Disk Space", totalSpace);
            addPropertyToTable(osTable, "Available Disk Space", freeSpace);
        }
    }
    
    void loadSummaryInformation()
    {
        summaryTable->setRowCount(0);
        
        // Create a concise OS/Distro summary line
        QString osInfo = QSysInfo::prettyProductName();
        QString arch = QSysInfo::currentCpuArchitecture();
        QString kernel = QSysInfo::kernelVersion();
        QString hostname = QSysInfo::machineHostName();
        
        // Get distribution info from /etc/os-release
        QString distroName;
        QString distroVersion;
        QFile releaseFile("/etc/os-release");
        if (releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&releaseFile);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.contains('=')) {
                    QStringList parts = line.split('=', Qt::SkipEmptyParts);
                    if (parts.size() >= 2) {
                        QString key = parts[0];
                        QString value = parts[1].remove('"');
                        
                        if (key == "NAME") {
                            distroName = value;
                        } else if (key == "VERSION") {
                            distroVersion = value;
                        }
                    }
                }
            }
        }
        
        // Construct the OS summary line
        QString osSummary = QString("%1 %2 (%3, Kernel %4)")
                          .arg(distroName.isEmpty() ? osInfo : distroName)
                          .arg(distroVersion)
                          .arg(arch)
                          .arg(kernel);
        
        addPropertyToTable(summaryTable, "Operating System", osSummary);
        addPropertyToTable(summaryTable, "Hostname", hostname);
        
        // Add uptime
        QFile uptimeFile("/proc/uptime");
        if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString uptimeStr = uptimeFile.readAll().split(' ')[0];
            double uptimeSeconds = uptimeStr.toDouble();
            int days = uptimeSeconds / 86400;
            int hours = (int(uptimeSeconds) % 86400) / 3600;
            int minutes = (int(uptimeSeconds) % 3600) / 60;
            
            QString formattedUptime = QString("%1d %2h %3m")
                                     .arg(days).arg(hours).arg(minutes);
            addPropertyToTable(summaryTable, "Uptime", formattedUptime);
        }
        
        // Add total memory
        QFile memFile("/proc/meminfo");
        if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&memFile);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.startsWith("MemTotal:")) {
                    QString memStr = line.split(QRegularExpression("\\s+"))[1];
                    double memKB = memStr.toDouble();
                    QString memFormatted = formatSize(memKB * 1024);
                    addPropertyToTable(summaryTable, "Total Memory", memFormatted);
                    break;
                }
            }
        }
        
        // Add current user and desktop environment
        QString user = qgetenv("USER");
        if (user.isEmpty()) user = qgetenv("USERNAME");
        addPropertyToTable(summaryTable, "User", user);
        
        QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
        if (desktop.isEmpty()) desktop = qgetenv("DESKTOP_SESSION");
        if (!desktop.isEmpty()) {
            addPropertyToTable(summaryTable, "Desktop", desktop);
        }
    }
    
    void addPropertyToTable(QTableWidget *table, const QString &property, const QString &value)
    {
        if (value.isEmpty()) return;
        
        int row = table->rowCount();
        table->insertRow(row);
        
        QTableWidgetItem *propertyItem = new QTableWidgetItem(property);
        QTableWidgetItem *valueItem = new QTableWidgetItem(value);
        
        // Apply special styling for Summary table
        if (table == summaryTable) {
            // Make property column bold
            QFont boldFont = propertyItem->font();
            boldFont.setBold(true);
            propertyItem->setFont(boldFont);
            
            // Make value column dark blue
            valueItem->setForeground(QBrush(QColor(0, 0, 139))); // Dark blue color
        }
        
        table->setItem(row, 0, propertyItem);
        table->setItem(row, 1, valueItem);
    }
    
    QString formatSize(double bytes)
    {
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024*1024) return QString::number(bytes/1024, 'f', 1) + " KB";
        if (bytes < 1024*1024*1024) return QString::number(bytes/(1024*1024), 'f', 1) + " MB";
        return QString::number(bytes/(1024*1024*1024), 'f', 1) + " GB";
    }

private:
    QString externalIPv4;
    QString externalIPv6;
    
    QTabWidget *tabWidget;
    QTableWidget *summaryTable;
    QTableWidget *osTable;
    QTableWidget *systemTable;
    QTableWidget *cpuTable;
    QTableWidget *memoryTable;
    QTableWidget *storageTable;
    QTableWidget *networkTable;
    QTableWidget *searchResultsTable;
    int searchResultsTabIndex;
    
    QPushButton *refreshButton;
    QLineEdit *searchEdit;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QTimer *networkRefreshTimer;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties for better system integration
    app.setApplicationName("LinInfoGUI");
    app.setApplicationDisplayName("Linux System Viewer");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("NalleBerg");
    app.setOrganizationDomain("nalle.no");
    
    // Set application icon globally for file handlers
    QStringList iconPaths = {
        "LinInfoGUI.png",
        "./LinInfoGUI.png",
        QApplication::applicationDirPath() + "/LinInfoGUI.png",
        QApplication::applicationDirPath() + "/../LinInfoGUI.png"
    };
    
    for (const QString &iconPath : iconPaths) {
        if (QFile::exists(iconPath)) {
            QIcon appIcon(iconPath);
            if (!appIcon.isNull()) {
                app.setWindowIcon(appIcon);
                qDebug() << "Global icon set from:" << iconPath;
                break;
            }
        }
    }
    
    LinInfoGUI window;
    window.show();
    
    return app.exec();
}

#include "LinInfoGUI.moc"

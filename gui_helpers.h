#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QLineEdit>
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QRegularExpression>
#include <QList>
#include <QDebug>
#include <QAbstractItemView>
#include <QVariant>
#include <QMetaType>
#include <QtGlobal>

// Search result structure
struct SearchResult {
    int tabIndex;
    QString tabName;
    QString rowData;
    int row;
    int column;
};

// Declare SearchResult as a Qt metatype for QVariant storage
Q_DECLARE_METATYPE(SearchResult)

// UI utility functions
void addRowToTable(QTableWidget* table, const QStringList& data);
void setupTableWidget(QTableWidget* table, const QStringList& headers);
void clearAllHighlighting(QList<QTableWidget*> tables);
void highlightMatchedText(QTableWidget* table, int row, int col, const QString& searchTerm, bool useRegex);

// Search functions
QList<SearchResult> performSearch(const QString& searchTerm, QList<QTableWidget*> tables, const QStringList& tabNames, bool useRegex);
void displaySearchResults(QTableWidget* searchTable, const QList<SearchResult>& results);
void navigateToSearchResult(QTabWidget* tabWidget, QList<QTableWidget*> tables, const SearchResult& result);

// Table styling functions
void styleTable(QTableWidget* table);
void styleSearchTable(QTableWidget* table);

// Inline implementations

inline void addRowToTable(QTableWidget* table, const QStringList& data)
{
    if (!table) return;
    
    int row = table->rowCount();
    table->insertRow(row);
    
    // Define colors: black bold for descriptions, purple for values
    QColor valueColor(56, 42, 126); // #382a7e
    QColor descriptionColor(0, 0, 0); // black
    
    for (int col = 0; col < data.size() && col < table->columnCount(); ++col) {
        QTableWidgetItem* item = new QTableWidgetItem(data[col]);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        
        if (col == 0) {
            // First column: descriptions in black and bold
            item->setForeground(QBrush(descriptionColor));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        } else {
            // Other columns: values in purple
            item->setForeground(QBrush(valueColor));
        }
        
        table->setItem(row, col, item);
    }
}

inline void setupTableWidget(QTableWidget* table, const QStringList& headers)
{
    if (!table) return;
    
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSortingEnabled(true);
    
    // Hide row numbers
    table->verticalHeader()->setVisible(false);
    
    // Style the table
    styleTable(table);
}

inline void styleTable(QTableWidget* table)
{
    if (!table) return;
    
    table->setStyleSheet(
        "QTableWidget {"
        "    gridline-color: #d0d0d0;"
        "    background-color: white;"
        "    alternate-background-color: #f5f5f5;"
        "    selection-background-color: #3399ff;"
        "}"
        "QTableWidget::item {"
        "    padding: 4px;"
        "    border: none;"
        "    background-color: transparent;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #3399ff;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #e0e0e0;"
        "    padding: 4px;"
        "    border: 1px solid #d0d0d0;"
        "    font-weight: bold;"
        "    color: black;"
        "}"
    );
}

inline void styleSearchTable(QTableWidget* table)
{
    if (!table) return;
    
    table->setStyleSheet(
        "QTableWidget {"
        "    gridline-color: #d0d0d0;"
        "    background-color: white;"
        "    alternate-background-color: #f9f9f9;"
        "    selection-background-color: #4CAF50;"
        "    color: black;"
        "}"
        "QTableWidget::item {"
        "    padding: 6px;"
        "    border: none;"
        "    color: black;"
        "    background-color: white;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "}"
        "QTableWidget::item:hover {"
        "    background-color: #e8f5e8;"
        "}"
        "QHeaderView::section {"
        "    background-color: #2196F3;"
        "    padding: 8px;"
        "    border: 1px solid #1976D2;"
        "    font-weight: bold;"
        "    color: white;"
        "}"
    );
}

inline void clearAllHighlighting(QList<QTableWidget*> tables)
{
    for (QTableWidget* table : tables) {
        if (!table) continue;
        
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                if (item) {
                    item->setBackground(QBrush());
                    item->setForeground(QBrush());
                }
            }
        }
    }
}

inline void highlightMatchedText(QTableWidget* table, int row, int col, const QString& searchTerm, bool useRegex = false)
{
    if (!table) return;
    
    QTableWidgetItem* item = table->item(row, col);
    if (!item) return;
    
    bool hasMatch = false;
    if (useRegex) {
        QRegularExpression regex(searchTerm, QRegularExpression::CaseInsensitiveOption);
        if (regex.isValid() && regex.match(item->text()).hasMatch()) {
            hasMatch = true;
        }
    } else {
        if (item->text().contains(searchTerm, Qt::CaseInsensitive)) {
            hasMatch = true;
        }
    }
    
    if (hasMatch) {
        item->setBackground(QBrush(QColor(255, 255, 0, 100))); // Light yellow highlight
        item->setForeground(QBrush(QColor(0, 0, 0))); // Black text
    }
}

inline QList<SearchResult> performSearch(const QString& searchTerm, QList<QTableWidget*> tables, const QStringList& tabNames, bool useRegex = false)
{
    QList<SearchResult> results;
    
    if (searchTerm.length() < 2) {
        return results; // Require at least 2 characters
    }
    
    // Prepare regex if needed
    QRegularExpression regex;
    if (useRegex) {
        regex.setPattern(searchTerm);
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (!regex.isValid()) {
            qDebug() << "Invalid regex pattern:" << searchTerm;
            return results; // Return empty results for invalid regex
        }
    }
    
    for (int tabIndex = 0; tabIndex < tables.size() && tabIndex < tabNames.size(); ++tabIndex) {
        QTableWidget* table = tables[tabIndex];
        if (!table) continue;
        
        QString tabName = tabNames[tabIndex];
        
        for (int row = 0; row < table->rowCount(); ++row) {
            QStringList rowData;
            bool hasMatch = false;
            
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                QString cellText = item ? item->text() : "";
                rowData << cellText;
                
                // Check for match based on search mode
                if (useRegex) {
                    if (regex.match(cellText).hasMatch()) {
                        hasMatch = true;
                    }
                } else {
                    if (cellText.contains(searchTerm, Qt::CaseInsensitive)) {
                        hasMatch = true;
                    }
                }
            }
            
            if (hasMatch) {
                SearchResult result;
                result.tabIndex = tabIndex;
                result.tabName = tabName;
                result.rowData = rowData.join(" | ");
                result.row = row;
                result.column = -1; // Will be set when highlighting
                
                results.append(result);
            }
        }
    }
    
    return results;
}

inline void displaySearchResults(QTableWidget* searchTable, const QList<SearchResult>& results)
{
    if (!searchTable) return;
    
    searchTable->setRowCount(0);
    
    for (int i = 0; i < results.size(); ++i) {
        const SearchResult& result = results[i];
        
        addRowToTable(searchTable, {
            result.tabName,
            result.rowData
        });
        
        // Store the original result data in the item for later use
        QTableWidgetItem* tabItem = searchTable->item(i, 0);
        if (tabItem) {
            tabItem->setData(Qt::UserRole, QVariant::fromValue(result));
        }
    }
}

inline void navigateToSearchResult(QTabWidget* tabWidget, QList<QTableWidget*> tables, const SearchResult& result)
{
    if (!tabWidget || result.tabIndex < 0 || result.tabIndex >= tables.size()) {
        return;
    }
    
    // Switch to the appropriate tab
    tabWidget->setCurrentIndex(result.tabIndex);
    
    // Get the table and select the row
    QTableWidget* targetTable = tables[result.tabIndex];
    if (targetTable && result.row >= 0 && result.row < targetTable->rowCount()) {
        targetTable->selectRow(result.row);
        targetTable->scrollToItem(targetTable->item(result.row, 0));
    }
}

// Table column setup helpers
inline QStringList getSummaryHeaders()
{
    return {"Component", "Information"};
}

inline QStringList getOSHeaders()
{
    return {"Property", "Value"};
}

inline QStringList getSystemHeaders()
{
    return {"Property", "Value"};
}

inline QStringList getCPUHeaders()
{
    return {"Property", "Value"};
}

inline QStringList getMemoryHeaders()
{
    return {"Property", "Value"};
}

inline QStringList getStorageHeaders()
{
    return {"Device", "Mount Point", "Size", "Used", "Available", "Use%", "Filesystem", "Type", "Details"};
}

inline QStringList getNetworkHeaders()
{
    return {"Interface", "Product", "Vendor", "IP Address", "Status", "Driver"};
}

inline QStringList getSearchHeaders()
{
    return {"Tab", "Match"};
}

// Table initialization functions
inline void initializeSummaryTable(QTableWidget* table)
{
    setupTableWidget(table, getSummaryHeaders());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

inline void initializeOSTable(QTableWidget* table)
{
    setupTableWidget(table, getOSHeaders());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

inline void initializeSystemTable(QTableWidget* table)
{
    setupTableWidget(table, getSystemHeaders());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

inline void initializeCPUTable(QTableWidget* table)
{
    setupTableWidget(table, getCPUHeaders());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

inline void initializeMemoryTable(QTableWidget* table)
{
    setupTableWidget(table, getMemoryHeaders());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

inline void initializeStorageTable(QTableWidget* table)
{
    setupTableWidget(table, getStorageHeaders());
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

inline void initializeNetworkTable(QTableWidget* table)
{
    setupTableWidget(table, getNetworkHeaders());
    // Allow user to resize columns to see full IPv6 addresses
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    // Set minimum column widths to accommodate IPv6 addresses
    table->horizontalHeader()->setMinimumSectionSize(120);
    // Make IP Address column wider by default for IPv6
    table->setColumnWidth(3, 200); // IP Address column
}

inline void initializeSearchTable(QTableWidget* table)
{
    setupTableWidget(table, getSearchHeaders());
    styleSearchTable(table);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

#endif // UI_HELPERS_H
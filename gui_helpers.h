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
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QList>
#include <QDebug>
#include <QAbstractItemView>
#include <QVariant>
#include <QMetaType>
#include <QtGlobal>
#include <QMenu>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QTimer>

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
        "    gridline-color: transparent;"
        "    background-color: white;"
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
        "    border: none;"
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
        "    gridline-color: transparent;"
        "    background-color: white;"
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
        "    border: none;"
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
    // Order requested: Device, Size, Used, Available, Use%, Mount Point, Filesystem, Type
    return {"Device", "Size", "Used", "Available", "Use%", "Mount Point", "Filesystem"};
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
    table->setColumnCount(getStorageHeaders().size());
    table->setHorizontalHeaderLabels(getStorageHeaders());
    table->verticalHeader()->setVisible(false);
    
    // Add essential table properties (from setupTableWidget but skip styleTable)
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSortingEnabled(true);
    
    // Apply CPU-style header styling (dark background, white text)
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
    
    // Apply minimal table styling (no problematic item padding)
    table->setStyleSheet(
        "QTableWidget {"
        "    gridline-color: transparent;"
        "    background-color: white;"
        "    selection-background-color: #3399ff;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #3399ff;"
        "    color: white;"
        "}"
    );
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

// UI template helpers (strict template derived from CPU tab)
inline void styleHeadlineLabel(QLabel* lbl)
{
    if (!lbl) return;
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 0px;");
}

inline void styleGeekButton(QPushButton* btn)
{
    if (!btn) return;
    btn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 11px; min-width: 80px; max-height: 22px;}"
        "QPushButton:hover { background-color: #2980b9; }"
    );
}

inline void applyMainLayoutDefaults(QVBoxLayout* layout)
{
    if (!layout) return;
    // Use minimal spacing and zero margins to match CPU tab default layout
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
}

// Helper to create the headline layout with a "Geek Mode" button so all tabs
// share the exact same headline placement and styling.
inline QHBoxLayout* createHeadlineWithGeek(QWidget* parent, const QString& title, QPushButton** outButton = nullptr)
{
    QHBoxLayout* headlineLayout = new QHBoxLayout();
    QLabel* headline = new QLabel(title);
    styleHeadlineLabel(headline);
    QPushButton* geekButton = new QPushButton(QCoreApplication::translate("gui_helpers", "Geek Mode"), parent);
    styleGeekButton(geekButton);
    // ensure exact height
    geekButton->setFixedHeight(22);
    headlineLayout->addWidget(headline);
    headlineLayout->addStretch();
    headlineLayout->addWidget(geekButton);
    if (outButton) *outButton = geekButton;
    return headlineLayout;
}

// Lightweight helper to add copy-on-selection (Ctrl+C) and right-click Copy to a QTableWidget.
// Installs an event filter for Ctrl+C and a context menu on the table's viewport.
class TableCopyHandler : public QObject
{
public:
    explicit TableCopyHandler(QTableWidget* tbl, QTimer* pauseTimer = nullptr)
        : QObject(tbl), table(tbl), pauseTimer(pauseTimer)
    {
        if (!table) return;
        // allow multi-cell selection and item-level selection so users can select arbitrary cells
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectItems);

        // ensure right-click on the viewport triggers our custom menu
        if (table->viewport()) {
            table->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(table->viewport(), &QWidget::customContextMenuRequested, [this](const QPoint &pos){ showMenu(table->viewport()->mapToGlobal(pos)); });
            table->viewport()->installEventFilter(this);
            // Also install event filter on the table itself so Ctrl+C delivered to table is caught
            table->installEventFilter(this);
        } else {
            table->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(table, &QWidget::customContextMenuRequested, [this](const QPoint &pos){ showMenu(table->mapToGlobal(pos)); });
            table->installEventFilter(this);
        }
    }

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override
    {
        if (!table) return QObject::eventFilter(obj, ev);
        // If user starts interacting (mouse press or key press for selection), pause auto-refresh
        if ((ev->type() == QEvent::MouseButtonPress) || (ev->type() == QEvent::KeyPress)) {
            pauseIfNeeded();
        }

        if (ev->type() == QEvent::KeyPress) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
            if (ke && ke->matches(QKeySequence::Copy)) {
                doCopy();
                resumeIfNeeded();
                return true;
            }
        }
        // Show context menu on right-click releases or context menu events
        if (ev->type() == QEvent::ContextMenu) {
            QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(ev);
            showMenu(ce->globalPos());
            return true;
        }
        if (ev->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* me = static_cast<QMouseEvent*>(ev);
            if (me && me->button() == Qt::RightButton) {
                QWidget* w = qobject_cast<QWidget*>(obj);
                if (w) showMenu(w->mapToGlobal(me->pos()));
                return true;
            }
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QTableWidget* table{nullptr};
    QTimer* pauseTimer{nullptr};
    bool paused{false};

    // pos is global screen coordinates
    void showMenu(const QPoint& pos)
    {
        if (!table) return;
        QMenu menu;
        QAction* copyAct = menu.addAction(QObject::tr("Copy"));
        QObject::connect(copyAct, &QAction::triggered, [this]() { doCopy(); });
        pauseIfNeeded();
        menu.exec(pos);
        // after menu closed, resume if we paused
        resumeIfNeeded();
    }

    void doCopy()
    {
        if (!table) return;
        QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
        if (ranges.isEmpty()) return;

        // map row -> map of column->text
        QMap<int, QMap<int, QString>> rowMap;
        for (const QTableWidgetSelectionRange &range : ranges) {
            for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
                for (int c = range.leftColumn(); c <= range.rightColumn(); ++c) {
                    QString cellText;
                    QTableWidgetItem* it = table->item(r, c);
                    if (it) cellText = it->text();
                    else if (QWidget* w = table->cellWidget(r, c)) {
                        // attempt to extract text from common widget types
                        if (QLabel* lab = qobject_cast<QLabel*>(w)) cellText = lab->text();
                        else if (QLineEdit* le = qobject_cast<QLineEdit*>(w)) cellText = le->text();
                        // fallback: empty
                    }
                    // Only insert if non-empty (but preserve empties inside selection)
                    rowMap[r].insert(c, cellText);
                }
            }
        }

        QStringList rowsOut;
        QList<int> rows = rowMap.keys();
        std::sort(rows.begin(), rows.end());
        for (int r : rows) {
            QMap<int, QString> &cols = rowMap[r];
            QList<int> colKeys = cols.keys();
            std::sort(colKeys.begin(), colKeys.end());
            QStringList colsText;
            for (int c : colKeys) colsText << cols.value(c);
            rowsOut << colsText.join('\t');
        }

        QString out = rowsOut.join('\n'); // Join rows with newline
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) clipboard->setText(out, QClipboard::Clipboard);
        // resume after copying
        resumeIfNeeded();
    }

    void pauseIfNeeded()
    {
        if (pauseTimer && !paused) {
            pauseTimer->stop();
            paused = true;
        }
    }

    void resumeIfNeeded()
    {
        if (pauseTimer && paused) {
            pauseTimer->start();
            paused = false;
        }
    }
};

inline void enableTableCopy(QTableWidget* table, QTimer* pauseTimer = nullptr)
{
    if (!table) return;
    // Handler is parented to table so it will be cleaned up automatically
    new TableCopyHandler(table, pauseTimer);
}

#endif // UI_HELPERS_H
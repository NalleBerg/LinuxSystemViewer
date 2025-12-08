#include "geek_search.h"
#include <QApplication>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QStyle>
#include <QDebug>

GeekSearchDialog::GeekSearchDialog(QWidget* parent)
    : QDialog(parent)
    , m_hasSearched(false)
    , m_targetTable(nullptr)
    , m_isFiltered(false)
{
    setWindowTitle(tr("Geek Mode Search"));
    setModal(false); // Allow interaction with other windows
    setWindowFlags(Qt::Tool); // Tool window, not staying on top
    resize(800, 600);
    
    setupUI();
    
    // Collect all available data on startup
    m_allData = collectAllGeekData();
    
    updateSearchScopeVisibility();
    enableDisableControls();
}

void GeekSearchDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Search input row
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Enter search term..."));
    
    m_searchButton = new QPushButton(tr("Search"));
    m_searchButton->setToolTip(tr("Search"));
    m_searchButton->setMaximumWidth(80);
    
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(m_searchButton);
    mainLayout->addLayout(searchLayout);
    
    // Search options in a grid layout
    QGridLayout* optionsLayout = new QGridLayout();
    
    // Row 1: Search Type (Contains/Exact Match)
    QGroupBox* searchTypeGroup = new QGroupBox(tr("Search Type"));
    QHBoxLayout* searchTypeLayout = new QHBoxLayout(searchTypeGroup);
    
    m_searchTypeGroup = new QButtonGroup(this);
    m_containsRadio = new QRadioButton(tr("Contains"));
    m_exactMatchRadio = new QRadioButton(tr("Exact match"));
    m_containsRadio->setChecked(true); // Default
    
    m_searchTypeGroup->addButton(m_containsRadio, 0);
    m_searchTypeGroup->addButton(m_exactMatchRadio, 1);
    
    searchTypeLayout->addWidget(m_containsRadio);
    searchTypeLayout->addWidget(m_exactMatchRadio);
    searchTypeLayout->addStretch();
    
    // Row 2: Case Sensitivity
    QGroupBox* caseSensitivityGroup = new QGroupBox(tr("Case Sensitivity"));
    QHBoxLayout* caseSensitivityLayout = new QHBoxLayout(caseSensitivityGroup);
    
    m_caseSensitivityGroup = new QButtonGroup(this);
    m_caseInsensitiveRadio = new QRadioButton(tr("Case insensitive"));
    m_caseSensitiveRadio = new QRadioButton(tr("Case sensitive"));
    m_caseInsensitiveRadio->setChecked(true); // Default
    
    m_caseSensitivityGroup->addButton(m_caseInsensitiveRadio, 0);
    m_caseSensitivityGroup->addButton(m_caseSensitiveRadio, 1);
    
    caseSensitivityLayout->addWidget(m_caseInsensitiveRadio);
    caseSensitivityLayout->addWidget(m_caseSensitiveRadio);
    caseSensitivityLayout->addStretch();
    
    // Row 3: Search Scope (New Search/Search in Results) - Initially hidden
    m_searchScopeWidget = new QGroupBox(tr("Search Scope"));
    QHBoxLayout* searchScopeLayout = new QHBoxLayout(m_searchScopeWidget);
    
    m_searchScopeGroup = new QButtonGroup(this);
    m_newSearchRadio = new QRadioButton(tr("New search"));
    m_searchInResultsRadio = new QRadioButton(tr("Search in results"));
    m_newSearchRadio->setChecked(true); // Default
    
    m_searchScopeGroup->addButton(m_newSearchRadio, 0);
    m_searchScopeGroup->addButton(m_searchInResultsRadio, 1);
    
    searchScopeLayout->addWidget(m_newSearchRadio);
    searchScopeLayout->addWidget(m_searchInResultsRadio);
    searchScopeLayout->addStretch();
    
    // Regex checkbox
    QGroupBox* regexGroup = new QGroupBox(tr("Advanced"));
    QHBoxLayout* regexLayout = new QHBoxLayout(regexGroup);
    m_regexCheckbox = new QCheckBox(tr("Regular Expression (Regex)"));
    regexLayout->addWidget(m_regexCheckbox);
    regexLayout->addStretch();
    
    // Add all option groups to grid
    optionsLayout->addWidget(searchTypeGroup, 0, 0);
    optionsLayout->addWidget(caseSensitivityGroup, 0, 1);
    optionsLayout->addWidget(m_searchScopeWidget, 1, 0);
    optionsLayout->addWidget(regexGroup, 1, 1);
    
    mainLayout->addLayout(optionsLayout);
    
    // Results tree
    m_resultsTree = new QTreeWidget();
    QStringList headers;
    headers << tr("Tab") << tr("Section") << tr("Property") << tr("Value") << tr("Context");
    m_resultsTree->setHeaderLabels(headers);
    m_resultsTree->setRootIsDecorated(false);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setSortingEnabled(true);
    
    // Auto-resize columns to content
    for (int i = 0; i < headers.size(); ++i) {
        m_resultsTree->resizeColumnToContents(i);
    }
    
    mainLayout->addWidget(m_resultsTree);
    
    // Connections
    connect(m_searchButton, &QPushButton::clicked, this, &GeekSearchDialog::performSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &GeekSearchDialog::performSearch);
    
    connect(m_searchTypeGroup, &QButtonGroup::buttonClicked,
            this, &GeekSearchDialog::onSearchTypeChanged);
    connect(m_caseSensitivityGroup, &QButtonGroup::buttonClicked,
            this, &GeekSearchDialog::onCaseSensitivityChanged);
    connect(m_searchScopeGroup, &QButtonGroup::buttonClicked,
            this, &GeekSearchDialog::onSearchScopeChanged);
    connect(m_regexCheckbox, &QCheckBox::toggled, this, &GeekSearchDialog::onRegexToggled);
    
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked, 
            this, &GeekSearchDialog::onResultItemDoubleClicked);
}

void GeekSearchDialog::performSearch()
{
    // If we have a target table, use table filtering instead
    if (m_targetTable) {
        filterTargetTable();
        return;
    }

    QString searchTerm = m_searchEdit->text().trimmed();
    if (searchTerm.isEmpty()) {
        return;
    }
    
    GeekSearchOptions options = getCurrentSearchOptions();
    options.searchTerm = searchTerm;
    
    // Determine which dataset to search in
    QList<GeekSearchResult> datasetToSearch;
    if (options.searchScope == GeekSearchOptions::NewSearch || !m_hasSearched) {
        // Search in all data
        datasetToSearch = m_allData;
    } else {
        // Search in current results
        datasetToSearch = m_currentResults;
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    try {
        QList<GeekSearchResult> results = performSearchOnDataSet(datasetToSearch, options);
        
        m_currentResults = results;
        m_hasSearched = true;
        
        populateResultsTree(results);
        updateSearchScopeVisibility();
        
        // Update status
        if (results.isEmpty()) {
            // No results found - status removed for compactness
        } else if (results.size() == 1) {
            // Found 1 result - status removed for compactness
        } else {
            // Found multiple results - status removed for compactness
        }
        
    } catch (const QRegularExpression& /*regexError*/) {
        // Invalid regex - status removed for compactness
        QApplication::restoreOverrideCursor();
        return;
    }
    
    QApplication::restoreOverrideCursor();
}

QList<GeekSearchResult> GeekSearchDialog::performSearchOnDataSet(
    const QList<GeekSearchResult>& dataSet, 
    const GeekSearchOptions& options)
{
    QList<GeekSearchResult> results;
    
    for (const GeekSearchResult& item : dataSet) {
        bool matches = false;
        
        // Search in both property and value fields
        QString searchText = item.property + " " + item.value;
        
        if (options.useRegex) {
            // Use regular expression
            QRegularExpression regex(options.searchTerm);
            if (options.caseSensitivity == GeekSearchOptions::CaseSensitive) {
                regex.setPatternOptions(QRegularExpression::NoPatternOption);
            } else {
                regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            }
            
            if (!regex.isValid()) {
                // Invalid regex - throw exception to be caught by caller
                throw regex;
            }
            
            matches = searchText.contains(regex);
            
        } else if (options.searchType == GeekSearchOptions::ExactMatch) {
            // Exact match
            if (options.caseSensitivity == GeekSearchOptions::CaseSensitive) {
                matches = searchText == options.searchTerm;
            } else {
                matches = searchText.compare(options.searchTerm, Qt::CaseInsensitive) == 0;
            }
            
        } else {
            // Contains search (default)
            if (options.caseSensitivity == GeekSearchOptions::CaseSensitive) {
                matches = searchText.contains(options.searchTerm, Qt::CaseSensitive);
            } else {
                matches = searchText.contains(options.searchTerm, Qt::CaseInsensitive);
            }
        }
        
        if (matches) {
            results.append(item);
        }
    }
    
    return results;
}

void GeekSearchDialog::populateResultsTree(const QList<GeekSearchResult>& results)
{
    m_resultsTree->clear();
    
    for (const GeekSearchResult& result : results) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, result.tabName);
        item->setText(1, result.section);
        item->setText(2, result.property);
        item->setText(3, result.value);
        item->setText(4, result.context);
        
        // Store the full result data for potential future use
        item->setData(0, Qt::UserRole, QVariant::fromValue(result));
        
        m_resultsTree->addTopLevelItem(item);
    }
    
    // Auto-resize columns to content
    for (int i = 0; i < 5; ++i) {
        m_resultsTree->resizeColumnToContents(i);
    }
}

GeekSearchOptions GeekSearchDialog::getCurrentSearchOptions() const
{
    GeekSearchOptions options;
    
    // Search type
    if (m_exactMatchRadio->isChecked()) {
        options.searchType = GeekSearchOptions::ExactMatch;
    } else {
        options.searchType = GeekSearchOptions::Contains;
    }
    
    // Case sensitivity
    if (m_caseSensitiveRadio->isChecked()) {
        options.caseSensitivity = GeekSearchOptions::CaseSensitive;
    } else {
        options.caseSensitivity = GeekSearchOptions::CaseInsensitive;
    }
    
    // Search scope
    if (m_searchInResultsRadio->isChecked()) {
        options.searchScope = GeekSearchOptions::SearchInResults;
    } else {
        options.searchScope = GeekSearchOptions::NewSearch;
    }
    
    // Regex
    options.useRegex = m_regexCheckbox->isChecked();
    
    return options;
}

void GeekSearchDialog::updateSearchScopeVisibility()
{
    // Show search scope options only when we have multiple search results
    // (either in table filtering mode with filtered results, or normal mode with multiple results)
    bool showScope = false;
    if (m_targetTable) {
        // Table filtering mode - show only if we have filtered results with multiple items
        showScope = m_isFiltered && m_currentFilteredData.size() > 1;
    } else {
        // Normal mode - show if we have searched and have multiple results
        showScope = m_hasSearched && m_currentResults.size() > 1;
    }
    m_searchScopeWidget->setVisible(showScope);
}

void GeekSearchDialog::enableDisableControls()
{
    // When regex is enabled, disable search type and case sensitivity
    bool regexEnabled = m_regexCheckbox->isChecked();
    
    m_containsRadio->setEnabled(!regexEnabled);
    m_exactMatchRadio->setEnabled(!regexEnabled);
    m_caseInsensitiveRadio->setEnabled(!regexEnabled);
    m_caseSensitiveRadio->setEnabled(!regexEnabled);
}

// Slot implementations
void GeekSearchDialog::onSearchTypeChanged()
{
    // Nothing special needed here for now
}

void GeekSearchDialog::onCaseSensitivityChanged()
{
    // Nothing special needed here for now  
}

void GeekSearchDialog::onSearchScopeChanged()
{
    // Nothing special needed here for now
}

void GeekSearchDialog::onRegexToggled(bool enabled)
{
    enableDisableControls();
    
    // Update placeholder text to give user hints
    if (enabled) {
        m_searchEdit->setPlaceholderText(tr("Enter regular expression... (e.g., 'Intel.*WiFi')"));
    } else {
        m_searchEdit->setPlaceholderText(tr("Enter search term..."));
    }
}

void GeekSearchDialog::onResultItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    // Get the stored result data
    QVariant data = item->data(0, Qt::UserRole);
    if (!data.canConvert<GeekSearchResult>()) return;
    
    GeekSearchResult result = data.value<GeekSearchResult>();
    
    // Show details in a message box for now
    QString details = tr("Search Result Details\n\n"
                        "Tab: %1\n"
                        "Section: %2\n"
                        "Property: %3\n"
                        "Value: %4\n"
                        "Context: %5")
                        .arg(result.tabName)
                        .arg(result.section)
                        .arg(result.property)
                        .arg(result.value)
                        .arg(result.context);
    
    QMessageBox::information(this, tr("Result Details"), details);
}

// Static function to collect all geek data from available dialogs
QList<GeekSearchResult> GeekSearchDialog::collectAllGeekData()
{
    QList<GeekSearchResult> allData;
    
    // Sample data for testing - in real implementation this would collect from actual geek dialogs
    allData.append(GeekSearchResult("CPU", "Processor Info", "Vendor", "Intel Corporation"));
    allData.append(GeekSearchResult("CPU", "Processor Info", "Model", "Intel(R) Core(TM) i7-10700K"));
    allData.append(GeekSearchResult("CPU", "Processor Info", "Architecture", "x86_64"));
    allData.append(GeekSearchResult("Memory", "RAM Info", "Total Memory", "32 GB"));
    allData.append(GeekSearchResult("Graphics", "GPU Info", "Vendor", "NVIDIA Corporation"));
    allData.append(GeekSearchResult("Network", "WiFi Info", "Hardware", "Intel Wi-Fi 6E AX210"));
    
    return allData;
}

// Helper function to collect data from a QTreeWidget
void GeekSearchDialog::collectFromTreeWidget(QTreeWidget* tree, const QString& tabName, 
                                           const QString& section, QList<GeekSearchResult>& results)
{
    if (!tree) return;
    
    QTreeWidgetItemIterator it(tree);
    while (*it) {
        QTreeWidgetItem* item = *it;
        
        if (item->columnCount() >= 2) {
            QString property = item->text(0);
            QString value = item->text(1);
            
            if (!property.isEmpty() && !value.isEmpty()) {
                results.append(GeekSearchResult(tabName, section, property, value));
            }
        }
        
        ++it;
    }
}
void GeekSearchDialog::setTargetTable(QTableWidget* table)
{
    m_targetTable = table;
    if (table) {
        storeOriginalTableData();
        // Hide the results tree since we'll filter the external table
        m_resultsTree->hide();
        // Hide search scope initially - will show when multiple results
        m_searchScopeWidget->setVisible(false);
        // Make dialog more compact without status label
        resize(400, 220);
        // Keep button as "Search" only
        // Search button already has text from constructor
        // Enable Enter key in search field
        connect(m_searchEdit, &QLineEdit::returnPressed, this, &GeekSearchDialog::performSearch);
    }
}

void GeekSearchDialog::closeEvent(QCloseEvent* event)
{
    // Reset table filtering when dialog closes - but only if table is still valid
    if (m_targetTable && m_isFiltered) {
        // Check if table parent is still valid before accessing
        QWidget* tableParent = m_targetTable->parentWidget();
        if (tableParent) {
            resetTargetTable();
        } else {
            // Parent is gone, just clear our references
            m_targetTable = nullptr;
            m_isFiltered = false;
        }
    }
    
    // Call parent implementation
    QDialog::closeEvent(event);
}

void GeekSearchDialog::storeOriginalTableData()
{
    if (!m_targetTable) return;
    
    m_originalTableData.clear();
    for (int row = 0; row < m_targetTable->rowCount(); ++row) {
        QString property = m_targetTable->item(row, 0) ? m_targetTable->item(row, 0)->text() : QString();
        QString value = m_targetTable->item(row, 1) ? m_targetTable->item(row, 1)->text() : QString();
        m_originalTableData.append(qMakePair(property, value));
    }
    m_currentFilteredData = m_originalTableData;
}

void GeekSearchDialog::filterTargetTable()
{
    if (!m_targetTable) {
        qDebug() << "Warning: filterTargetTable called but m_targetTable is null";
        return;
    }
    
    // Check if table is still valid by checking if parent exists
    QWidget* tableParent = m_targetTable->parentWidget();
    if (!tableParent) {
        qDebug() << "Warning: target table parent is null - clearing table reference";
        m_targetTable = nullptr;
        return;
    }
    
    QString searchText = m_searchEdit->text();
    if (searchText.isEmpty()) return;
    
    QVector<QPair<QString, QString>> dataToSearch;
    
    // Determine which data to search based on search scope
    if (m_searchInResultsRadio->isChecked() && m_isFiltered && !m_currentFilteredData.isEmpty()) {
        dataToSearch = m_currentFilteredData;
    } else {
        dataToSearch = m_originalTableData;
    }
    
    QVector<QPair<QString, QString>> filteredResults;
    
    // Perform search
    for (const auto& item : dataToSearch) {
        if (matchesTableSearchCriteria(item.first, item.second, searchText)) {
            filteredResults.append(item);
        }
    }
    
    // Update table with filtered results
    m_targetTable->setRowCount(filteredResults.size());
    for (int i = 0; i < filteredResults.size(); ++i) {
        m_targetTable->setItem(i, 0, new QTableWidgetItem(filteredResults[i].first));
        m_targetTable->setItem(i, 1, new QTableWidgetItem(filteredResults[i].second));
    }
    
    m_currentFilteredData = filteredResults;
    m_isFiltered = true;
    
    // Update search scope visibility based on result count
    updateSearchScopeVisibility();
}

void GeekSearchDialog::resetTargetTable()
{
    if (!m_targetTable) return;
    
    // Restore original data
    m_targetTable->setRowCount(m_originalTableData.size());
    for (int i = 0; i < m_originalTableData.size(); ++i) {
        m_targetTable->setItem(i, 0, new QTableWidgetItem(m_originalTableData[i].first));
        m_targetTable->setItem(i, 1, new QTableWidgetItem(m_originalTableData[i].second));
    }
    
    m_currentFilteredData = m_originalTableData;
    m_isFiltered = false;
    
    // Hide search scope when reset
    m_searchScopeWidget->setVisible(false);
    
    // Clear search text
    m_searchEdit->clear();
}

bool GeekSearchDialog::matchesTableSearchCriteria(const QString& property, const QString& value, const QString& searchText)
{
    Qt::CaseSensitivity caseSensitivity = m_caseInsensitiveRadio->isChecked() ? 
                                         Qt::CaseInsensitive : Qt::CaseSensitive;
    
    QString combinedText = property + " " + value;
    
    if (m_regexCheckbox->isChecked()) {
        QRegularExpression regex(searchText);
        if (caseSensitivity == Qt::CaseInsensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        return regex.match(combinedText).hasMatch();
    }
    
    if (m_exactMatchRadio->isChecked()) {
        return property.compare(searchText, caseSensitivity) == 0 ||
               value.compare(searchText, caseSensitivity) == 0;
    } else {
        return combinedText.contains(searchText, caseSensitivity);
    }
}

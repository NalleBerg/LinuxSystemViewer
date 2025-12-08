#ifndef GEEK_SEARCH_H
#define GEEK_SEARCH_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <QButtonGroup>
#include <QLabel>
#include <QSplitter>
#include <QTextEdit>
#include <QRegularExpression>
#include <QString>
#include <QList>
#include <QMetaType>
#include <QCloseEvent>
#include <QGridLayout>
#include <QIcon>
#include <QStyle>
#include <QApplication>
#include <QRect>
#include <QScreen>
#include <QGroupBox>

// Structure to hold a search result with context
struct GeekSearchResult {
    QString tabName;        // Which tab (CPU, Memory, etc.)
    QString section;        // Which section within the tab
    QString property;       // Property name
    QString value;          // Property value
    QString context;        // Additional context if needed
    
    GeekSearchResult() = default;
    GeekSearchResult(const QString& tab, const QString& sect, 
                     const QString& prop, const QString& val,
                     const QString& ctx = QString())
        : tabName(tab), section(sect), property(prop), value(val), context(ctx) {}
};

struct GeekSearchOptions {
    enum SearchType { Contains, ExactMatch };
    enum CaseSensitivity { CaseInsensitive, CaseSensitive };
    enum SearchScope { NewSearch, SearchInResults };
    
    SearchType searchType;
    CaseSensitivity caseSensitivity;
    SearchScope searchScope;
    bool useRegex;
    QString searchTerm;
    
    GeekSearchOptions() 
        : searchType(Contains), caseSensitivity(CaseInsensitive), 
          searchScope(NewSearch), useRegex(false) {}
};

class GeekSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GeekSearchDialog(QWidget* parent = nullptr);
    void setTargetTable(QTableWidget* table);
    
    static QList<GeekSearchResult> collectAllGeekData();

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void performSearch();

private slots:
    void onSearchTypeChanged();
    void onCaseSensitivityChanged();
    void onSearchScopeChanged();
    
private:
    void setupUI();
    void setupSearchOptions();
    void setupResultsDisplay();
    
    GeekSearchOptions getCurrentSearchOptions() const;
    QList<GeekSearchResult> performSearchOnDataset(const QList<GeekSearchResult>& dataset, const GeekSearchOptions& options);
    void displaySearchResults(const QList<GeekSearchResult>& results);
    void updateSearchScopeVisibility();
    void enableDisableControls();
    bool matchesSearchCriteria(const GeekSearchResult& result, const GeekSearchOptions& options);
    
    // Missing method declarations from old implementation
    void populateResultsTree(const QList<GeekSearchResult>& results);
    QList<GeekSearchResult> performSearchOnDataSet(const QList<GeekSearchResult>& dataset, const GeekSearchOptions& options);
    void onRegexToggled(bool enabled);
    void onResultItemDoubleClicked(QTreeWidgetItem* item, int column);
    void collectFromTreeWidget(QTreeWidget* tree, const QString& tabName, const QString& section, QList<GeekSearchResult>& results);
    
    // UI Elements
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    
    // Radio button groups
    QButtonGroup* m_searchTypeGroup;
    QRadioButton* m_containsRadio;
    QRadioButton* m_exactMatchRadio;
    
    QButtonGroup* m_caseSensitivityGroup;
    QRadioButton* m_caseInsensitiveRadio;
    QRadioButton* m_caseSensitiveRadio;
    
    QButtonGroup* m_searchScopeGroup;
    QRadioButton* m_newSearchRadio;
    QRadioButton* m_searchInResultsRadio;
    QWidget* m_searchScopeWidget; // To show/hide this row
    
    QCheckBox* m_regexCheckbox;
    
    // Results display
    QTreeWidget* m_resultsTree;
    
    // Target table filtering support
    QTableWidget* m_targetTable;
    QVector<QPair<QString, QString>> m_originalTableData;
    QVector<QPair<QString, QString>> m_currentFilteredData;
    bool m_isFiltered;
    
    // Search state
    QList<GeekSearchResult> m_allData;      // All available data
    QList<GeekSearchResult> m_currentResults; // Current search results
    bool m_hasSearched;
    
    // Helper methods for table filtering
    void storeOriginalTableData();
    void filterTargetTable();
    void resetTargetTable();
    bool matchesTableSearchCriteria(const QString& property, const QString& value, const QString& searchText);
    
    static void appendWidgetDataToResults(const QWidget* widget, const QString& tabName,
                                    const QString& section, QList<GeekSearchResult>& results);
};

Q_DECLARE_METATYPE(GeekSearchResult)

#endif // GEEK_SEARCH_H

#ifndef GEEK_SEARCH_INTEGRATION_H
#define GEEK_SEARCH_INTEGRATION_H

#include <QPushButton>
#include <QHBoxLayout>
#include <QDialog>
#include <QTableWidget>
#include <QWidget>

#include "geek_search.h"
#include <QPushButton>
#include <QDialog>
#include <QTableWidget>
#include <QHBoxLayout>

class GeekSearchIntegration
{
public:
    // Static method to add a search button to any geek dialog's button layout
    static QPushButton* addSearchButtonToGeekDialog(QHBoxLayout* buttonLayout, QDialog* parentDialog, QTableWidget* table);
    
    // Static method to show the search dialog
    static void showGeekSearchDialog(QWidget* parent = nullptr, QTableWidget* table = nullptr);
    
private:
    static GeekSearchDialog* s_searchDialog; // Singleton search dialog
};

#endif // GEEK_SEARCH_INTEGRATION_H
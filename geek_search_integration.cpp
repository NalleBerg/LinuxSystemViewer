#include "geek_search_integration.h"
#include "geek_search.h"
#include <QApplication>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDialog>
#include <QTableWidget>
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QRect>

// Static member definition
GeekSearchDialog* GeekSearchIntegration::s_searchDialog = nullptr;

QPushButton* GeekSearchIntegration::addSearchButtonToGeekDialog(QHBoxLayout* buttonLayout, QDialog* parentDialog, QTableWidget* table)
{
    QPushButton* searchBtn = new QPushButton(QObject::tr("Search"));
    
    // Remove icon to save space - just text
    searchBtn->setToolTip(QObject::tr("Search Geek mode data"));
    
    // Add the button after the first button (usually Rescan) but before the stretch
    if (buttonLayout->count() > 0) {
        buttonLayout->insertWidget(1, searchBtn);
    } else {
        buttonLayout->addWidget(searchBtn);
    }
    
    // Connect to show search dialog
    QObject::connect(searchBtn, &QPushButton::clicked, [parentDialog, table]() {
        showGeekSearchDialog(parentDialog, table);
    });
    
    return searchBtn;
}

void GeekSearchIntegration::showGeekSearchDialog(QWidget* parent, QTableWidget* table)
{
    // Recreate dialog each time to avoid dangling parent/table pointers
    if (s_searchDialog) {
        delete s_searchDialog;
        s_searchDialog = nullptr;
    }
    
    s_searchDialog = new GeekSearchDialog(parent);
    
    // Clean up search dialog when parent is destroyed
    if (parent) {
        QObject::connect(parent, &QObject::destroyed, []() {
            if (s_searchDialog) {
                s_searchDialog->deleteLater();
                s_searchDialog = nullptr;
            }
        });
    }
    
    // Set target table for filtering if provided
    if (table) {
        s_searchDialog->setTargetTable(table);
    }
    
    // Show and position the search dialog smartly
    s_searchDialog->show();
    
    // Position dialog to the right of parent, but keep on screen
    if (parent) {
        QRect parentGeom = parent->geometry();
        QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
        
        int x = parentGeom.right() + 10; // 10px gap
        int y = parentGeom.top();
        
        // Ensure dialog stays within screen bounds
        if (x + s_searchDialog->width() > screenGeom.right()) {
            x = parentGeom.left() - s_searchDialog->width() - 10; // Position to left instead
        }
        if (x < screenGeom.left()) {
            x = screenGeom.left(); // Clamp to left edge
        }
        if (y + s_searchDialog->height() > screenGeom.bottom()) {
            y = screenGeom.bottom() - s_searchDialog->height(); // Clamp to bottom
        }
        if (y < screenGeom.top()) {
            y = screenGeom.top(); // Clamp to top
        }
        
        s_searchDialog->move(x, y);
    }
    
    s_searchDialog->raise();
    s_searchDialog->activateWindow();
}

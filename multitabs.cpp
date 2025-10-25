#include "multitabs.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QScreen>
#include <QApplication>

MultiRowTabWidget::MultiRowTabWidget(QWidget* parent)
    : QWidget(parent), m_currentIndex(-1)
{
    setupUI();
    qDebug() << "MultiRowTabWidget: Initialized";
}

void MultiRowTabWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // Create scroll area for tabs - but we'll size it properly to avoid scrolling
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Never show horizontal scrollbar
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // Never show vertical scrollbar
    m_scrollArea->setFrameStyle(QFrame::NoFrame); // Remove border
    
    // Tab container
    m_tabContainer = new QWidget();
    m_tabLayout = new QGridLayout(m_tabContainer);
    m_tabLayout->setContentsMargins(5, 5, 5, 5);
    m_tabLayout->setSpacing(3);
    
    m_scrollArea->setWidget(m_tabContainer);
    m_mainLayout->addWidget(m_scrollArea);
    
    // Stacked widget for content
    m_stackedWidget = new QStackedWidget();
    m_mainLayout->addWidget(m_stackedWidget);
}

QSize MultiRowTabWidget::calculateOptimalSize()
{
    // Simple fixed size - let the main window decide
    return QSize(850, 480);
}

void MultiRowTabWidget::addTab(QWidget* widget, const QString& title)
{
    QPushButton* button = new QPushButton(title);
    button->setCheckable(true);
    button->setMinimumHeight(40); // Increased height to prevent cutting off
    button->setMaximumHeight(40);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Style the button
    button->setStyleSheet(
        "QPushButton {"
        "  background-color: #3498db;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 12px;" // Increased padding
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2980b9;"
        "}"
        "QPushButton:checked {"
        "  background-color: #1f1971;"
        "}"
    );
    
    connect(button, &QPushButton::clicked, this, &MultiRowTabWidget::onTabButtonClicked);
    
    m_tabs.append(TabInfo(title, widget, button));
    m_stackedWidget->addWidget(widget);
    
    updateTabLayout();
    
    if (m_currentIndex == -1) {
        setCurrentIndex(0);
    }
    
    qDebug() << "MultiRowTabWidget: Added tab" << title << "Total tabs:" << m_tabs.size();
}

void MultiRowTabWidget::updateTabLayout()
{
    if (m_tabs.isEmpty()) return;
    
    int tabCount = m_tabs.size();
    
    // Clear existing layout
    while (QLayoutItem* item = m_tabLayout->takeAt(0)) {
        // Don't delete the widget, just remove from layout
        item->widget()->setParent(m_tabContainer);
    }
    
    // Force exactly 2 rows
    const int maxRows = 2;
    int tabsPerRow = (tabCount + maxRows - 1) / maxRows; // Ceiling division
    
    qDebug() << "MultiRowTabWidget: Arranging" << tabCount << "tabs in" << maxRows << "rows with" << tabsPerRow << "tabs per row";
    
    // Calculate tab width based on available space
    int availableWidth = width() - 30; // Account for margins and spacing
    if (availableWidth <= 0) availableWidth = 810; // Default based on 850px window (850-40=810)
    
    int tabWidth = availableWidth / tabsPerRow;
    if (tabWidth < 100) tabWidth = 100; // Increased minimum width
    if (tabWidth > 220) tabWidth = 220; // Increased max width for better fit
    
    // Place buttons in grid layout
    for (int i = 0; i < tabCount; ++i) {
        int row = i / tabsPerRow;
        int col = i % tabsPerRow;
        
        QPushButton* button = m_tabs[i].button;
        button->setMinimumWidth(tabWidth);
        button->setMaximumWidth(tabWidth);
        
        m_tabLayout->addWidget(button, row, col);
    }
    
    // Set layout properties
    m_tabLayout->setRowStretch(0, 0); // Don't stretch rows
    m_tabLayout->setRowStretch(1, 0);
    
    // Calculate the required height for the tab area
    int buttonHeight = 40; // Height of each button
    int spacing = m_tabLayout->spacing();
    int margins = m_tabLayout->contentsMargins().top() + m_tabLayout->contentsMargins().bottom();
    int requiredHeight = (maxRows * buttonHeight) + ((maxRows - 1) * spacing) + margins + 10; // +10 for extra safety
    
    // Set the scroll area to the exact height needed
    m_scrollArea->setMinimumHeight(requiredHeight);
    m_scrollArea->setMaximumHeight(requiredHeight);
    
    // Force the tab container to the right size
    m_tabContainer->setMinimumSize(availableWidth, requiredHeight - 10);
    
    qDebug() << "MultiRowTabWidget: Tab area height set to:" << requiredHeight;
    
    updateTabStyling();
}

void MultiRowTabWidget::updateTabStyling()
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        QPushButton* button = m_tabs[i].button;
        button->setChecked(i == m_currentIndex);
    }
}

void MultiRowTabWidget::onTabButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].button == button) {
            setCurrentIndex(i);
            emit tabClicked(i);
            break;
        }
    }
}

void MultiRowTabWidget::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) {
        return;
    }
    
    m_currentIndex = index;
    m_stackedWidget->setCurrentIndex(index);
    updateTabStyling();
    
    emit currentChanged(index);
    
    qDebug() << "MultiRowTabWidget: Current index changed to" << index;
}

int MultiRowTabWidget::currentIndex() const
{
    return m_currentIndex;
}

int MultiRowTabWidget::count() const
{
    return m_tabs.size();
}

QWidget* MultiRowTabWidget::widget(int index) const
{
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs[index].widget;
    }
    return nullptr;
}

void MultiRowTabWidget::removeTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    
    QWidget* widget = m_tabs[index].widget;
    QPushButton* button = m_tabs[index].button;
    
    m_tabs.removeAt(index);
    m_stackedWidget->removeWidget(widget);
    
    delete button;
    
    if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
    }
    
    if (m_currentIndex >= 0) {
        setCurrentIndex(m_currentIndex);
    }
    
    updateTabLayout();
}

void MultiRowTabWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0, this, &MultiRowTabWidget::updateTabLayout);
}
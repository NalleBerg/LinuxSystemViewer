#ifndef MULTITABS_H
#define MULTITABS_H

#include <QWidget>
#include <functional>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QResizeEvent>
#include <QScreen>
#include <QApplication>

struct TabInfo {
    QString originalName; // canonical tab id from TAB_CONFIGS (e.g. "Summary")
    QString title;
    QWidget* widget;
    QPushButton* button;
    
    TabInfo(const QString& orig, const QString& t, QWidget* w, QPushButton* b)
        : originalName(orig), title(t), widget(w), button(b) {}
};

class MultiRowTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MultiRowTabWidget(QWidget* parent = nullptr);
    
    void addTab(QWidget* widget, const QString& title, const QString& originalName = QString());
    // Re-apply translated titles using a translation function that maps
    // an original tab name (from TAB_CONFIGS) to a translated label.
    void retranslateTabs(const std::function<QString(const QString&)>& translatorFunc);
    // Attempt to shut down any background activity in the tab widgets so
    // they can be safely deleted. This will call TabWidgetBase::shutdown()
    // for each tab if applicable.
    void shutdownTabs();
    void setCurrentIndex(int index);
    int currentIndex() const;
    int count() const;
    QWidget* widget(int index) const;
    void removeTab(int index);

signals:
    void currentChanged(int index);
    void tabClicked(int index);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTabButtonClicked();
    void updateTabLayout();

private:
    void setupUI();
    void updateTabStyling();
    QSize calculateOptimalSize();
    
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_tabContainer;
    QGridLayout* m_tabLayout;
    QStackedWidget* m_stackedWidget;
    
    QList<TabInfo> m_tabs;
    int m_currentIndex;
};

#endif // MULTITABS_H
#ifndef MULTITABS_H
#define MULTITABS_H

#include <QWidget>
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
    QString title;
    QWidget* widget;
    QPushButton* button;
    
    TabInfo(const QString& t, QWidget* w, QPushButton* b) 
        : title(t), widget(w), button(b) {}
};

class MultiRowTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MultiRowTabWidget(QWidget* parent = nullptr);
    
    void addTab(QWidget* widget, const QString& title);
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
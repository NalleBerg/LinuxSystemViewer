#ifndef STORAGE_TAB_H
#define STORAGE_TAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include <QFutureWatcher>
#include <QVariantMap>
#include <QPushButton>
#include <QDialog>
#include "tab_widget_base.h"

class GeekStorageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekStorageDialog(QWidget* parent = nullptr);
    void fillTable();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QTableWidget* table;
    QTimer* refreshTimer;
};

class StorageTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit StorageTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private slots:
    void onParseFinished();
    void applyParsedPartitions(const QVariantMap& parsed);
    void showGeekMode();

private:
    QPushButton* geekButton{nullptr};
    void refreshData();
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QWidget* diskInfoContainer;
    QVBoxLayout* diskInfoLayout;
    QVector<QTableWidget*> diskTables;
    QVector<QLabel*> diskLabels;
    QTimer* refreshTimer{nullptr};
    QFutureWatcher<QVariantMap>* parseWatcher{nullptr};
    QString m_pendingOutput;
    
    void populateStorageTable(); 
    QString formatSizeLocale(const QString& sizeStr);
    QTableWidget* createDiskTable();
    QVariantMap parseStorageData(const QString& output);
    QString formatTextWithTruncation(const QString& text, int maxWidth, const QFont& font);
    QTableWidgetItem* createColoredTextItem(const QString& text);
};

#endif // STORAGE_TAB_H
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QProgressBar>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QStorageInfo>
#include <QIcon>
#include <QFont>

#define VERSION "0.1.0"

// Just include Qt headers needed for basic compilation test
#include <QTableWidget>
#include <QTableWidgetItem>

class LinInfoGUI : public QMainWindow
{
    Q_OBJECT

public:
    LinInfoGUI(QWidget *parent = nullptr);
    ~LinInfoGUI();

private slots:
    void onRefreshClicked();

private:
    void setupUI();
    
    // UI Components
    QTabWidget *tabWidget;
    QTableWidget *summaryTable;
    QTableWidget *osTable;
    QTableWidget *systemTable;
    QTableWidget *cpuTable;
    QTableWidget *memoryTable;
    QTableWidget *storageTable;
    QTableWidget *networkTable;
    QTableWidget *searchTable;
    QPushButton *refreshButton;
    QLineEdit *searchField;
    QLabel *statusLabel;
    QProgressBar *progressBar;
};

LinInfoGUI::LinInfoGUI(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

LinInfoGUI::~LinInfoGUI()
{
}

void LinInfoGUI::setupUI()
{
    setWindowTitle(QString("Linux System Viewer - V. %1").arg(VERSION));
    setMinimumSize(800, 500);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Simple test layout
    refreshButton = new QPushButton("Refresh", this);
    connect(refreshButton, &QPushButton::clicked, this, &LinInfoGUI::onRefreshClicked);
    
    statusLabel = new QLabel("Modular Build Test - Ready", this);
    
    tabWidget = new QTabWidget(this);
    
    // Create simple tables
    summaryTable = new QTableWidget(2, 2, this);
    summaryTable->setItem(0, 0, new QTableWidgetItem("Test"));
    summaryTable->setItem(0, 1, new QTableWidgetItem("Modular headers work!"));
    
    tabWidget->addTab(summaryTable, "Summary");
    
    mainLayout->addWidget(refreshButton);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(tabWidget);
}

void LinInfoGUI::onRefreshClicked()
{
    statusLabel->setText("Refresh clicked - modular build successful!");
}

// Required for QObject with Q_OBJECT macro
#include "LinInfoGUI.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("LinInfoGUI");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("NalleBerg");
    app.setOrganizationDomain("nalle.no");
    
    LinInfoGUI window;
    window.show();
    
    return app.exec();
}
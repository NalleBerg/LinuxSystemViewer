#include "generic_tab.h"
#include <QVBoxLayout>
#include <QObject>
#include <QTextEdit>
#include <QFont>
#include <QLabel>
#include <QDebug>

GenericTab::GenericTab(const QString& tabName, const QString& command, 
                      bool hasGeekMode, const QString& geekCommand, QWidget* parent)
    : TabWidgetBase(tabName, command, hasGeekMode, geekCommand, parent)
{
    qDebug() << "GenericTab: Constructor called for:" << tabName;
    initializeTab();
    qDebug() << "GenericTab: Constructor finished for:" << tabName;
}

QWidget* GenericTab::createUserFriendlyView()
{
    qDebug() << "GenericTab: createUserFriendlyView called for:" << m_tabName;
    
    m_outputDisplay = new QTextEdit();
    m_outputDisplay->setReadOnly(true);
    m_outputDisplay->setFont(QFont("monospace", 10));
    m_outputDisplay->setPlainText(QObject::tr("Loading %1 information...").arg(m_tabName));
    
    m_outputDisplay->setStyleSheet(
        "QTextEdit {"
        "  background-color: #f8f9fa;"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "  padding: 10px;"
        "}"
    );
    
    qDebug() << "GenericTab: createUserFriendlyView completed for:" << m_tabName;
    return m_outputDisplay;
}

void GenericTab::parseOutput(const QString& output)
{
    qDebug() << "GenericTab: parseOutput called for:" << m_tabName;
    
    if (m_outputDisplay) {
    QString formattedOutput = QObject::tr("=== %1 Information ===\n\n%2").arg(m_tabName).arg(output);
    m_outputDisplay->setPlainText(formattedOutput);
    }
    
    qDebug() << "GenericTab: parseOutput completed for:" << m_tabName;
}
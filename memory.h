#ifndef MEMORY_H
#define MEMORY_H

#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QRegularExpression>
#include <QTimer>
#include <QObject>
#include "gui_helpers.h"

// Forward declarations
class QTableWidget;

// Memory information functions
void loadMemoryInformation(QTableWidget* memoryTable, const QJsonObject &memoryData = QJsonObject());
QString getMemoryInfo();

// Memory data structure for real-time updates
struct MemoryStats {
    qint64 totalKB = 0;
    qint64 availableKB = 0;
    qint64 usedKB = 0;
    qint64 swapTotalKB = 0;
    qint64 swapFreeKB = 0;
    qint64 swapUsedKB = 0;
    double usagePercent = 0.0;
    double swapUsagePercent = 0.0;
};

// Memory management class for live updates
class MemoryMonitor : public QObject {
    Q_OBJECT

public:
    explicit MemoryMonitor(QTableWidget* table, QObject* parent = nullptr);
    ~MemoryMonitor();
    
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

public slots:
    void updateMemoryData();

private:
    QTableWidget* memoryTable;
    QTimer* updateTimer;
    bool monitoring;
    
    MemoryStats readMemoryStats();
    void updateTable(const MemoryStats& stats);
};

// Helper function to get memory info for summary
inline QString getMemoryInfo()
{
    QFile memInfoFile("/proc/meminfo");
    if (!memInfoFile.open(QIODevice::ReadOnly)) {
        return "Unknown";
    }
    
    QString content = memInfoFile.readAll();
    memInfoFile.close();
    
    // Extract total memory
    QRegularExpression memTotalRegex(R"(MemTotal:\s*(\d+)\s*kB)");
    QRegularExpressionMatch match = memTotalRegex.match(content);
    if (match.hasMatch()) {
        qint64 memKB = match.captured(1).toLongLong();
        double memGB = memKB / (1024.0 * 1024.0);
        return QString::number(memGB, 'f', 2) + " GB";
    }
    
    return "Unknown";
}

// Main memory information loading function
inline void loadMemoryInformation(QTableWidget* memoryTable, const QJsonObject &memoryData)
{
    if (!memoryTable) return;
    
    memoryTable->setRowCount(0);
    
    // Get memory information from /proc/meminfo
    QFile memInfoFile("/proc/meminfo");
    if (memInfoFile.open(QIODevice::ReadOnly)) {
        QString content = memInfoFile.readAll();
        memInfoFile.close();
        
        // Parse memory information
        QRegularExpression memTotalRegex(R"(MemTotal:\s*(\d+)\s*kB)");
        QRegularExpression memAvailRegex(R"(MemAvailable:\s*(\d+)\s*kB)");
        QRegularExpression swapTotalRegex(R"(SwapTotal:\s*(\d+)\s*kB)");
        QRegularExpression swapFreeRegex(R"(SwapFree:\s*(\d+)\s*kB)");
        
        QRegularExpressionMatch totalMatch = memTotalRegex.match(content);
        QRegularExpressionMatch availMatch = memAvailRegex.match(content);
        QRegularExpressionMatch swapTotalMatch = swapTotalRegex.match(content);
        QRegularExpressionMatch swapFreeMatch = swapFreeRegex.match(content);
        
        if (totalMatch.hasMatch()) {
            qint64 totalKB = totalMatch.captured(1).toLongLong();
            double totalGB = totalKB / (1024.0 * 1024.0);
            addRowToTable(memoryTable, {"Total RAM", QString::number(totalGB, 'f', 2) + " GB"});
            
            if (availMatch.hasMatch()) {
                qint64 availKB = availMatch.captured(1).toLongLong();
                qint64 usedKB = totalKB - availKB;
                double usedGB = usedKB / (1024.0 * 1024.0);
                double availGB = availKB / (1024.0 * 1024.0);
                
                addRowToTable(memoryTable, {"Used RAM", QString::number(usedGB, 'f', 2) + " GB"});
                addRowToTable(memoryTable, {"Available RAM", QString::number(availGB, 'f', 2) + " GB"});
                
                double usagePercent = (double(usedKB) / totalKB) * 100.0;
                addRowToTable(memoryTable, {"Usage", QString::number(usagePercent, 'f', 1) + "%"});
            }
        }
        
        if (swapTotalMatch.hasMatch()) {
            qint64 swapTotalKB = swapTotalMatch.captured(1).toLongLong();
            if (swapTotalKB > 0) {
                double swapTotalGB = swapTotalKB / (1024.0 * 1024.0);
                addRowToTable(memoryTable, {"Total Swap", QString::number(swapTotalGB, 'f', 2) + " GB"});
                
                if (swapFreeMatch.hasMatch()) {
                    qint64 swapFreeKB = swapFreeMatch.captured(1).toLongLong();
                    qint64 swapUsedKB = swapTotalKB - swapFreeKB;
                    double swapUsedGB = swapUsedKB / (1024.0 * 1024.0);
                    
                    addRowToTable(memoryTable, {"Used Swap", QString::number(swapUsedGB, 'f', 2) + " GB"});
                }
            }
        }
    }
    
    // Add lshw memory data if available
    if (memoryData.contains("size")) {
        double bytes = memoryData["size"].toDouble();
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        addRowToTable(memoryTable, {"Detected RAM", QString::number(gb, 'f', 2) + " GB"});
    }
}

#endif // MEMORY_H
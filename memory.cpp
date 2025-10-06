#include "memory.h"
#include "gui_helpers.h"

MemoryMonitor::MemoryMonitor(QTableWidget* table, QObject* parent) 
    : QObject(parent), memoryTable(table), monitoring(false)
{
    updateTimer = new QTimer(this);
    updateTimer->setInterval(1000); // 1 second updates
    connect(updateTimer, &QTimer::timeout, this, &MemoryMonitor::updateMemoryData);
}

MemoryMonitor::~MemoryMonitor()
{
    stopMonitoring();
}

void MemoryMonitor::startMonitoring()
{
    if (!monitoring && memoryTable) {
        monitoring = true;
        updateTimer->start();
        updateMemoryData(); // Initial update
    }
}

void MemoryMonitor::stopMonitoring()
{
    if (monitoring) {
        monitoring = false;
        updateTimer->stop();
    }
}

bool MemoryMonitor::isMonitoring() const
{
    return monitoring;
}

MemoryStats MemoryMonitor::readMemoryStats()
{
    MemoryStats stats;
    
    QFile memInfoFile("/proc/meminfo");
    if (!memInfoFile.open(QIODevice::ReadOnly)) {
        return stats;
    }
    
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
        stats.totalKB = totalMatch.captured(1).toLongLong();
        
        if (availMatch.hasMatch()) {
            stats.availableKB = availMatch.captured(1).toLongLong();
            stats.usedKB = stats.totalKB - stats.availableKB;
            stats.usagePercent = (double(stats.usedKB) / stats.totalKB) * 100.0;
        }
    }
    
    if (swapTotalMatch.hasMatch()) {
        stats.swapTotalKB = swapTotalMatch.captured(1).toLongLong();
        
        if (swapFreeMatch.hasMatch()) {
            qint64 swapFreeKB = swapFreeMatch.captured(1).toLongLong();
            stats.swapUsedKB = stats.swapTotalKB - swapFreeKB;
            if (stats.swapTotalKB > 0) {
                stats.swapUsagePercent = (double(stats.swapUsedKB) / stats.swapTotalKB) * 100.0;
            }
        }
    }
    
    return stats;
}

void MemoryMonitor::updateTable(const MemoryStats& stats)
{
    if (!memoryTable) return;
    
    memoryTable->setRowCount(0);
    
    if (stats.totalKB > 0) {
        double totalGB = stats.totalKB / (1024.0 * 1024.0);
        double usedGB = stats.usedKB / (1024.0 * 1024.0);
        double availGB = stats.availableKB / (1024.0 * 1024.0);
        
        addRowToTable(memoryTable, {"Total RAM", QString::number(totalGB, 'f', 2) + " GB"});
        addRowToTable(memoryTable, {"Used RAM", QString::number(usedGB, 'f', 2) + " GB"});
        addRowToTable(memoryTable, {"Available RAM", QString::number(availGB, 'f', 2) + " GB"});
        addRowToTable(memoryTable, {"Usage", QString::number(stats.usagePercent, 'f', 1) + "%"});
        
        if (stats.swapTotalKB > 0) {
            double swapTotalGB = stats.swapTotalKB / (1024.0 * 1024.0);
            double swapUsedGB = stats.swapUsedKB / (1024.0 * 1024.0);
            
            addRowToTable(memoryTable, {"Total Swap", QString::number(swapTotalGB, 'f', 2) + " GB"});
            addRowToTable(memoryTable, {"Used Swap", QString::number(swapUsedGB, 'f', 2) + " GB"});
        }
    }
}

void MemoryMonitor::updateMemoryData()
{
    MemoryStats stats = readMemoryStats();
    updateTable(stats);
}
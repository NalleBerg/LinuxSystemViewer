#ifndef LOG_HELPER_H
#define LOG_HELPER_H

// The debug logger is optional and can be compiled out using the
// CMake option -DLSV_ENABLE_DEBUG_LOGGER=ON. When compiled out, appendLog
// is a no-op and will not create any files or have side-effects.
#ifdef LSV_ENABLE_DEBUG_LOGGER

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QString>
#include <cstdlib>
#include <cstring>

inline void appendLog(const QString& msg)
{
    // Unconditional logging for debugging - will be conditional again after issue is resolved
    static const bool enabled = true;
    if (!enabled) {
        return; // no-op when debugging is not explicitly enabled
    }

    // Write to home directory for easy access
    static bool firstCall = true;
    QString home = QString::fromLocal8Bit(qgetenv("HOME"));
    if (home.isEmpty()) home = QDir::tempPath();
    QString logPath = home + QDir::separator() + "lsv-debug.log";
    QFile f(logPath);
    if (firstCall) {
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&f);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << msg << "\n";
            f.close();
        }
        firstCall = false;
    } else {
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&f);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << msg << "\n";
            f.close();
        }
    }

}

#else

// Logging compiled out: no-op implementation
#include <QString>
inline void appendLog(const QString& /*msg*/) { }

#endif // LSV_ENABLE_DEBUG_LOGGER

#endif // LOG_HELPER_H

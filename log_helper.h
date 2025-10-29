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
    // Enable runtime logging by setting the environment variable LSV_DEBUG=1.
    const char* env = std::getenv("LSV_DEBUG");
    static const bool enabled = (env && (std::strcmp(env, "1") == 0 || std::strcmp(env, "true") == 0));
    if (!enabled) {
        return; // no-op when debugging is not explicitly enabled
    }

    // When enabled, write to the system temp directory to avoid creating
    // persistent files inside AppImage or user folders.
    static bool firstCall = true;
    QString tmp = QDir::tempPath() + QDir::separator() + "lsv-debug.log";
    QFile f(tmp);
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

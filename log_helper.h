#ifndef LOG_HELPER_H
#define LOG_HELPER_H

// The debug logger is optional and can be compiled out using the
// CMake option -DLSV_ENABLE_DEBUG_LOGGER=ON. When compiled out, appendLog
// is a no-op and will not create any files or have side-effects.
#ifdef LSV_ENABLE_DEBUG_LOGGER
// Debug logger disabled for release/cleanup
#endif

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <unistd.h> // for geteuid

inline void appendLog(const QString& msg)
{
	// Log to /tmp/lsv_debug.log if running as root, else to ./logs/lsv_debug.log
	static QString logPath;
	static bool firstCall = true;
	if (logPath.isEmpty()) {
		if (geteuid() == 0) {
			logPath = "/tmp/lsv_debug.log";
		} else {
			logPath = QDir::currentPath() + "/logs/lsv_debug.log";
		}
	}
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

#endif // LOG_HELPER_H
#ifndef LOG_HELPER_H
#define LOG_HELPER_H

// The debug logger is optional and can be compiled out using the
// CMake option -DLSV_ENABLE_DEBUG_LOGGER=ON. When compiled out, appendLog
// is a no-op and will not create any files or have side-effects.
#ifdef LSV_ENABLE_DEBUG_LOGGER
// Debug logger disabled for release/cleanup
#endif

#define appendLog(x) do {} while(0)



#endif // LOG_HELPER_H

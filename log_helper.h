#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

inline void appendLog(const QString& msg)
{
    // On first call, truncate the log so each run starts with a fresh file.
    static bool firstCall = true;
    QFile f("lsv-cli.log");
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

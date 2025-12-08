#ifndef SCREEN_H
#define SCREEN_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QScreen>

struct ScreenInfo {
    QString resolution;
    QString refreshRate;
    QString sizeInMm;
    QString manufacturer;
    QString orientation;
    double scaleFactor;
};

inline ScreenInfo loadScreenInformation() {
    ScreenInfo info;
    
    // Get primary screen from Qt
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (!primaryScreen) {
        info.resolution = "Unknown";
        info.refreshRate = "Unknown";
        info.sizeInMm = "Unknown";
        info.manufacturer = "";
        info.orientation = "Unknown";
        info.scaleFactor = 1.0;
        return info;
    }
    
    // Resolution
    QSize resolution = primaryScreen->size();
    info.resolution = QString("%1 x %2").arg(resolution.width()).arg(resolution.height());
    
    // Refresh rate
    qreal refreshRate = primaryScreen->refreshRate();
    info.refreshRate = QString::number(refreshRate, 'f', 2) + " Hz";
    
    // Physical size in mm
    QSizeF physicalSize = primaryScreen->physicalSize();
    info.sizeInMm = QString("%1 x %2 mm").arg(physicalSize.width(), 0, 'f', 1).arg(physicalSize.height(), 0, 'f', 1);
    
    // Scale factor (zoom level)
    info.scaleFactor = primaryScreen->devicePixelRatio();
    
    // Orientation
    Qt::ScreenOrientation orientation = primaryScreen->orientation();
    switch (orientation) {
        case Qt::PortraitOrientation:
            info.orientation = "Portrait";
            break;
        case Qt::LandscapeOrientation:
            info.orientation = "Landscape";
            break;
        case Qt::InvertedPortraitOrientation:
            info.orientation = "Inverted Portrait";
            break;
        case Qt::InvertedLandscapeOrientation:
            info.orientation = "Inverted Landscape";
            break;
        default:
            info.orientation = "Landscape"; // Default
            break;
    }
    
    // Try to get manufacturer from EDID via DRM
    QDir drmDir("/sys/class/drm");
    QStringList connectors = drmDir.entryList(QStringList() << "card*-*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& connector : connectors) {
        QFile statusFile(QString("/sys/class/drm/%1/status").arg(connector));
        if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString status = QTextStream(&statusFile).readLine().trimmed();
            statusFile.close();
            
            if (status == "connected") {
                // Try to read EDID
                QFile edidFile(QString("/sys/class/drm/%1/edid").arg(connector));
                if (edidFile.open(QIODevice::ReadOnly)) {
                    QByteArray edidData = edidFile.readAll();
                    edidFile.close();
                    
                    // Parse manufacturer from EDID (bytes 8-9, 3-letter code)
                    if (edidData.size() >= 18) {
                        // Manufacturer ID is encoded in bytes 8-9 (big-endian)
                        unsigned char byte1 = edidData[8];
                        unsigned char byte2 = edidData[9];
                        
                        // Extract 3 letters (5 bits each)
                        char letter1 = ((byte1 >> 2) & 0x1F) + 'A' - 1;
                        char letter2 = (((byte1 & 0x03) << 3) | ((byte2 >> 5) & 0x07)) + 'A' - 1;
                        char letter3 = (byte2 & 0x1F) + 'A' - 1;
                        
                        QString mfgCode = QString("%1%2%3").arg(letter1).arg(letter2).arg(letter3);
                        
                        // Try to get full name from descriptor blocks (bytes 54+, 72+, 90+, 108+)
                        QString mfgName;
                        for (int offset : {54, 72, 90, 108}) {
                            if (edidData.size() >= offset + 18) {
                                // Check if this is a display name descriptor (type 0xFC)
                                if (edidData[offset + 3] == (char)0xFC) {
                                    mfgName = QString::fromLatin1(edidData.mid(offset + 5, 13)).trimmed();
                                    break;
                                }
                            }
                        }
                        
                        if (!mfgName.isEmpty()) {
                            info.manufacturer = mfgName;
                        } else if (mfgCode.length() == 3) {
                            info.manufacturer = mfgCode;
                        }
                    }
                    
                    break; // Found connected display with EDID
                }
            }
        }
    }
    
    return info;
}

#endif // SCREEN_H

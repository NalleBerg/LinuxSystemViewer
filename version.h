// Single place to change the application version string
// To update the version: edit version.cpp
#pragma once

#include <QString>

// Version string - defined in version.cpp
// This is the ONLY place you need to change the version number
extern const char* LSV_VERSION;

// Helper to get a QString version
inline QString LSVVersionQString()
{
    return QString::fromUtf8(LSV_VERSION);
}
